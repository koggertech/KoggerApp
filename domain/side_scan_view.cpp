#include "side_scan_view.h"

#include <QtMath>


SideScanView::SideScanView(QObject* parent) :
    SceneObject(new SideScanViewRenderImplementation, parent),
    datasetPtr_(nullptr),
    scaleFactor_(10.0f),
    segFChannelId_(-1),
    segSChannelId_(-1)
{
    updateColorTable();
}

SideScanView::~SideScanView()
{

}

void SideScanView::updateDataSec()
{
    scaleFactor_ = 1.0f;

    if (!datasetPtr_) {
        qDebug() << "dataset is nullptr!";
        return;
    }

    auto renderImpl = RENDER_IMPL(SideScanView);
    updateChannelsIds();

    auto epochCount = datasetPtr_->size();
    if (epochCount < 4)
        return;

    ////////////////////////////////////////////////////////////////////
    // prepairing
    MatrixParams actualMatParams(lastMatParams_);
    MatrixParams newMatrixParams;


    QVector<QVector3D> newMeasLinesVertices_;
    QVector<int> newMeasLinesEvenIndices_;
    QVector<int> newMeasLinesOddIndices_;

    QVector<char> newIsOdds;
    QVector<int> newEpochIndxs;


    datasetPtr_->last();

    QVector<int> currUsedEpochIndx;


    //qDebug() << "lastCalcEpoch_" << lastCalcEpoch_ << "epochCount" << epochCount;


    for (int i = lastCalcEpoch_; i < epochCount; ++i) {
        //qDebug() << "i: " << i << ", epochCount: " << epochCount;

        if (auto epoch = datasetPtr_->fromIndex(i); epoch) {
            auto pos = epoch->getPositionGNSS().ned;
            auto yaw = epoch->yaw();
            if (isfinite(pos.n) && isfinite(pos.e) && isfinite(yaw)) {
                double azRad = qDegreesToRadians(yaw);
                if (auto segFCharts = epoch->chart(segFChannelId_); segFCharts) {
                    double leftAzRad = azRad - M_PI_2;
                    float lDist = segFCharts->range();



                    newMeasLinesVertices_.append(QVector3D(pos.n + lDist * qCos(leftAzRad), pos.e + lDist * qSin(leftAzRad),  0.0f));
                    newMeasLinesVertices_.append(QVector3D(pos.n, pos.e, 0.0f));


                    newMeasLinesEvenIndices_.append(currIndxSec_++);
                    newMeasLinesEvenIndices_.append(currIndxSec_++);


                    newIsOdds.append('0');
                    newEpochIndxs.append(i);
                }
                if (auto segSCharts = epoch->chart(segSChannelId_); segSCharts) {
                    double rightAzRad = azRad + M_PI_2;
                    float rDist = segSCharts ->range();



                    newMeasLinesVertices_.append(QVector3D(pos.n, pos.e, 0.0f));
                    newMeasLinesVertices_.append(QVector3D(pos.n + rDist * qCos(rightAzRad), pos.e + rDist * qSin(rightAzRad), 0.0f));


                    newMeasLinesOddIndices_.append(currIndxSec_++);
                    newMeasLinesOddIndices_.append(currIndxSec_++);



                    newIsOdds.append('1');
                    newEpochIndxs.append(i);
                }
            }
        }        


        currUsedEpochIndx.append(i);
    }


    lastCalcEpoch_ = epochCount;


    newMatrixParams = getMatrixParams(newMeasLinesVertices_);


    //if (!newMatrixParams.isValid()) {
    //    return;
    //}

    concatenateMatrixParameters(actualMatParams, newMatrixParams);


    bool meshUpdated = globalMesh_.concatenate(actualMatParams);


    if (meshUpdated) { // just debug messages
        qDebug() << "/// inserted start ///";
        qDebug() << "actual matrix:";
        actualMatParams.print(qDebug());
        qDebug() << "globalmesh :";
        globalMesh_.printMatrix();
        qDebug() << "/// inserted end ///";
    }






    ////////////////////////////////////////////////////////////////////









    /*

    ////////////////////////////////////////////////////////////////////
    // processing

    // height (use last, realloc)
    //int actualHMatSize = actualMatParams.hMatWidth * actualMatParams.hMatHeight;
    //int currHSize =  renderImpl->heightVertices_.size();
    //if (currHSize != actualHMatSize) {
    // //   qDebug() << "resize height: bef: " << currHSize << ", aft: " << actualHMatSize;
    //    renderImpl->heightVertices_.resize(actualHMatSize);
    //}
    //else {
    //  //  qDebug() << "dont resize height mat: " << actualHMatSize << renderImpl->heightVertices_.size();
    //}

    //uchar* imageData = image_.bits();
    //int bytesPerLine = image_.bytesPerLine();
    //int bytesInPix = bytesPerLine / image_.width();

    //if (lastMatParams_.hMatWidth != actualMatParams.hMatWidth ||// fill pos
    //    lastMatParams_.hMatHeight != actualMatParams.hMatHeight) {
    //    for (int i = 0; i < actualMatParams.hMatHeight; ++i) {
    //        for (int j = 0; j < actualMatParams.hMatWidth; ++j) {
    //            float x = actualMatParams.minX + j * heightStep_;
    //            float y = actualMatParams.minY + i * heightStep_;
    //            renderImpl->heightVertices_[i * actualMatParams.hMatWidth + j] = QVector3D(x, y, 0.0f);
    //        }
    //    }
    //}


    // main cycle, interpolation only to the next epoch, when the segment side matches
    for (int i = 0; i < newMeasLinesVertices_.size(); i += 2) { // 2 - step for 1 segment

        if (i + 8 >= newMeasLinesVertices_.size()) {
            break;
        }

        int segFBegVertIndx = i;
        int segFEndVertIndx = i + 1;
        int segSBegVertIndx = i + 2;
        int segSEndVertIndx = i + 3;
        int segFIndx = i / 2;
        int segSIndx = (i + 2) / 2;
        // going to next epoch if needed
        if (epochIndxs_[segFIndx] == epochIndxs_[segSIndx] ||
            isOdds_[segFIndx] != isOdds_[segSIndx]) {
            ++segSIndx;
            segSBegVertIndx += 2;
            segSEndVertIndx += 2;
        }

        // compliance check
        if (epochIndxs_[segFIndx] == epochIndxs_[segSIndx] ||
            isOdds_[segFIndx] != isOdds_[segSIndx]) {
            continue;
        }
        // epochs checking
        auto segFEpochPtr = datasetPtr_->fromIndex(epochIndxs_[segFIndx]); // _plot might be reallocated
        auto segSEpochPtr = datasetPtr_->fromIndex(epochIndxs_[segSIndx]);
        if (!segFEpochPtr || !segSEpochPtr) {
            continue;
        }
        Epoch segFEpoch = *segFEpochPtr;
        Epoch segSEpoch = *segFEpochPtr;


        // isOdd checking
        bool segFIsOdd = isOdds_[segFIndx] == '1';
        bool segSIsOdd = isOdds_[segSIndx] == '1';
        if (segFIsOdd != segSIsOdd) {
            continue;
        }
        // segments checking
        auto segFCharts = segFEpoch.chart(segFIsOdd ? segSChannelId_ : segFChannelId_);
        auto segSCharts = segSEpoch.chart(segSIsOdd ? segSChannelId_ : segFChannelId_);
        if (!segFCharts || !segSCharts) {
            continue;
        }
        // dist procs checking
        if (!isfinite(segFEpoch.distProccesing(segFIsOdd ? segFChannelId_ : segSChannelId_)) ||
            !isfinite(segSEpoch.distProccesing(segSIsOdd ? segFChannelId_ : segSChannelId_))) {
            continue;
        }



        // Bresenham
        // first segment
        QVector3D segFBegPoint = !segFIsOdd ? renderImpl->measLinesVertices_[segFBegVertIndx] : renderImpl->measLinesVertices_[segFEndVertIndx];
        QVector3D segSEngPoint = !segFIsOdd ? renderImpl->measLinesVertices_[segFEndVertIndx] : renderImpl->measLinesVertices_[segFBegVertIndx];
        //int segFX1 = std::min(actualMatParams.width  - 1, std::max(0, static_cast<int>(std::round((segFBegPoint.x() - actualMatParams.minX) * scaleFactor_))));
        //int segFY1 = std::min(actualMatParams.height - 1, std::max(0, static_cast<int>(std::round((segFBegPoint.y() - actualMatParams.minY) * scaleFactor_))));
        //int segFX2 = std::min(actualMatParams.width  - 1, std::max(0, static_cast<int>(std::round((segSEngPoint.x() - actualMatParams.minX) * scaleFactor_))));
        //int segFY2 = std::min(actualMatParams.height - 1, std::max(0, static_cast<int>(std::round((segSEngPoint.y() - actualMatParams.minY) * scaleFactor_))));
        //float segFPixTotDist = std::sqrt(std::pow(segFX2 - segFX1, 2) + std::pow(segFY2 - segFY1, 2));
        //int segFDx = std::abs(segFX2 - segFX1);
        //int segFDy = std::abs(segFY2 - segFY1);
        //int segFSx = (segFX1 < segFX2) ? 1 : -1;
        //int segFSy = (segFY1 < segFY2) ? 1 : -1;
        //int segFErr = segFDx - segFDy;
        // second segment
        QVector3D segSBegPoint = !segSIsOdd ? renderImpl->measLinesVertices_[segSBegVertIndx] : renderImpl->measLinesVertices_[segSEndVertIndx];
        QVector3D segSEndPoint = !segSIsOdd ? renderImpl->measLinesVertices_[segSEndVertIndx] : renderImpl->measLinesVertices_[segSBegVertIndx];
        //int segSX1 = std::min(actualMatParams.width  - 1, std::max(0, static_cast<int>(std::round((segSBegPoint.x() - actualMatParams.minX) * scaleFactor_))));
        //int segSY1 = std::min(actualMatParams.height - 1, std::max(0, static_cast<int>(std::round((segSBegPoint.y() - actualMatParams.minY) * scaleFactor_))));
        //int segSX2 = std::min(actualMatParams.width  - 1, std::max(0, static_cast<int>(std::round((segSEndPoint.x() - actualMatParams.minX) * scaleFactor_))));
        //int segSY2 = std::min(actualMatParams.height - 1, std::max(0, static_cast<int>(std::round((segSEndPoint.y() - actualMatParams.minY) * scaleFactor_))));
        //float segSPixTotDist = std::sqrt(std::pow(segSX2 - segSX1, 2) + std::pow(segSY2 - segSY1, 2));
        //int segSDx = std::abs(segSX2 - segSX1);
        //int segSDy = std::abs(segSY2 - segSY1);
        //int segSSx = (segSX1 < segSX2) ? 1 : -1;
        //int segSSy = (segSY1 < segSY2) ? 1 : -1;
        //int segSErr = segSDx - segSDy;

        // pixel length checking
        //if (!checkLength(segFPixTotDist) ||
        //    !checkLength(segSPixTotDist)) {
        //    continue;
        //}

        float segFDistProc = -1.0f * static_cast<float>(segFEpoch.distProccesing(segFIsOdd ? segFChannelId_ : segSChannelId_));
        float segSDistProc = -1.0f * static_cast<float>(segSEpoch.distProccesing(segSIsOdd ? segFChannelId_ : segSChannelId_));
        float segFDistX = segSEngPoint.x() - segFBegPoint.x();
        float segFDistY = segSEngPoint.y() - segFBegPoint.y();
        float segSDistX = segSEndPoint.x() - segSBegPoint.x();
        float segSDistY = segSEndPoint.y() - segSBegPoint.y();

        while (true) { // line passing
            // first segment
            float segFPixCurrDist = std::sqrt(std::pow(segFX1 - segFX2, 2) + std::pow(segFY1 - segFY2, 2));
            float segFProgress = std::min(1.0f, segFPixCurrDist / segFPixTotDist);
            QVector3D segFPixPos(segFBegPoint.x() + segFProgress * segFDistX, segFBegPoint.y() + segFProgress * segFDistY, segFDistProc);
            QVector3D segFBoatPos(segFEpoch.getPositionGNSS().ned.n, segFEpoch.getPositionGNSS().ned.e, 0.0f);
            auto segFColorIndx = getColorIndx(segFCharts, static_cast<int>(std::floor(segFPixPos.distanceToPoint(segFBoatPos) * amplitudeCoeff_)));

            // second segment, calc corresponding progress using smoothed interpolation
            float segSCorrProgress = std::min(1.0f, segFPixCurrDist / segFPixTotDist * segSPixTotDist / segFPixTotDist);
            QVector3D segSPixPos(segSBegPoint.x() + segSCorrProgress * segSDistX,    segSBegPoint.y() + segSCorrProgress * segSDistY,     segSDistProc);
            QVector3D segSBoatPos(segSEpoch.getPositionGNSS().ned.n, segSEpoch.getPositionGNSS().ned.e, 0.0f);
            auto segSColorIndx  = getColorIndx(segSCharts, static_cast<int>(std::floor(segSPixPos.distanceToPoint(segSBoatPos) * amplitudeCoeff_)));

            // color interpolation between two pixels
            int interpX1 = std::min(actualMatParams.width  - 1, std::max(0, static_cast<int>(std::round((segFPixPos.x() - actualMatParams.minX) * scaleFactor_))));
            int interpY1 = std::min(actualMatParams.height - 1, std::max(0, static_cast<int>(std::round((segFPixPos.y() - actualMatParams.minY) * scaleFactor_))));
            int interpX2 = std::min(actualMatParams.width  - 1, std::max(0, static_cast<int>(std::round((segSPixPos.x() - actualMatParams.minX) * scaleFactor_))));
            int interpY2 = std::min(actualMatParams.height - 1, std::max(0, static_cast<int>(std::round((segSPixPos.y() - actualMatParams.minY) * scaleFactor_))));
            int interpDistX = interpX2 - interpX1;
            int interpDistY = interpY2 - interpY1;
            float interpPixTotDist = std::sqrt(std::pow(interpX2 - interpX1, 2) + std::pow(interpY2 - interpY1, 2));
            // height matrix
            int uinterpX1 = std::min(actualMatParams.unscaledWidth  - 1, std::max(0, static_cast<int>(std::round((segFPixPos.x() - actualMatParams.minX)))));
            int uinterpY1 = std::min(actualMatParams.unscaledHeight - 1, std::max(0, static_cast<int>(std::round((segFPixPos.y() - actualMatParams.minY)))));
            int uinterpX2 = std::min(actualMatParams.unscaledWidth  - 1, std::max(0, static_cast<int>(std::round((segSPixPos.x() - actualMatParams.minX)))));
            int uinterpY2 = std::min(actualMatParams.unscaledHeight - 1, std::max(0, static_cast<int>(std::round((segSPixPos.y() - actualMatParams.minY)))));
            int uinterpDistX = uinterpX2 - uinterpX1;
            int uinterpDistY = uinterpY2 - uinterpY1;



            if (checkLength(interpPixTotDist)) {
                for (int step = 0; step <= interpPixTotDist; ++step) {
                    float interpProgress = static_cast<float>(step) / interpPixTotDist;
                    int interpX = interpX1 + interpProgress * interpDistX;
                    int interpY = interpY1 + interpProgress * interpDistY;
                    auto interpColorIndx = static_cast<int>((1 - interpProgress) * segFColorIndx + interpProgress * segSColorIndx);
                    // height matrix
                    int uinterpX = uinterpX1 + interpProgress * uinterpDistX;
                    int uinterpY = uinterpY1 + interpProgress * uinterpDistY;

                    for (int offsetX = -interpLineWidth_; offsetX <= interpLineWidth_; ++offsetX) {
                        for (int offsetY = -interpLineWidth_; offsetY <= interpLineWidth_; ++offsetY) {
                            int applyInterpX = std::min(actualMatParams.width  - 1, std::max(0, interpX + offsetX));
                            int applyInterpY = std::min(actualMatParams.height - 1, std::max(0, interpY + offsetY));
                            uchar* pixPtr = imageData + applyInterpY * bytesPerLine + applyInterpX * bytesInPix;
                            *pixPtr = colorTable_[interpColorIndx];
                            // height matrix
                            int uApplyInterpX = std::min(actualMatParams.unscaledWidth  - 1, std::max(0, uinterpX + offsetX));
                            int uApplyInterpY = std::min(actualMatParams.unscaledHeight - 1, std::max(0, uinterpY + offsetY));
                            int heightVerticesIndx = (uApplyInterpY / heightStep_) * actualMatParams.hMatWidth + (uApplyInterpX / heightStep_);
                            renderImpl->heightVertices_[heightVerticesIndx][2] = segSPixPos.z();
                        }
                    }
                }
            }

            // break at the end of the first segment
            if (segFX1 == segFX2 && segFY1 == segFY2) {
                break;
            }

            // Bresenham
            int segFE2 = 2 * segFErr;
            if (segFE2 > -segFDy) {
                segFErr -= segFDy;
                segFX1 += segFSx;
            }
            if (segFE2 < segFDx) {
                segFErr += segFDx;
                segFY1 += segFSy;
            }
            int segSE2 = 2 * segSErr;
            if (segSE2 > -segSDy) {
                segSErr -= segSDy;
                segSX1 += segSSx;
            }
            if (segSE2 < segSDx) {
                segSErr += segSDx;
                segSY1 += segSSy;
            }
        }
    }

    // textureVertices, heightIndices
    renderImpl->textureVertices_.clear();
    renderImpl->heightIndices_.clear();

    for (int i = 0; i < actualMatParams.hMatHeight ; ++i) {
        for (int j = 0; j < actualMatParams.hMatWidth ; ++j) {
            renderImpl->textureVertices_.append(QVector2D(float(j) / (actualMatParams.hMatWidth - 1), float(i) / (actualMatParams.hMatHeight - 1)));

            int topLeft = i * actualMatParams.hMatWidth + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * actualMatParams.hMatWidth + j;
            int bottomRight = bottomLeft + 1;

            if (qFuzzyCompare(1.0f, 1.0f + renderImpl->heightVertices_[topLeft].z()) || // someone zero
                qFuzzyCompare(1.0f, 1.0f + renderImpl->heightVertices_[topRight].z()) ||
                qFuzzyCompare(1.0f, 1.0f + renderImpl->heightVertices_[bottomLeft].z()) ||
                qFuzzyCompare(1.0f, 1.0f + renderImpl->heightVertices_[bottomRight].z())) {
                continue;
            }

            renderImpl->heightIndices_.append(topLeft);     // 1--3
            renderImpl->heightIndices_.append(bottomLeft);  // | /
            renderImpl->heightIndices_.append(topRight);    // 2
            renderImpl->heightIndices_.append(topRight);    //    1
            renderImpl->heightIndices_.append(bottomLeft);  //  / |
            renderImpl->heightIndices_.append(bottomRight); // 2--3
        }
    }    

    // grid
    QVector<QVector3D> grid;
    for (int i = 0; i < renderImpl->heightIndices_.size(); i += 6) {
        QVector3D A = renderImpl->heightVertices_[renderImpl->heightIndices_[i]];
        QVector3D B = renderImpl->heightVertices_[renderImpl->heightIndices_[i + 1]];
        QVector3D C = renderImpl->heightVertices_[renderImpl->heightIndices_[i + 2]];
        QVector3D D = renderImpl->heightVertices_[renderImpl->heightIndices_[i + 5]];
        A.setZ(A.z() + 0.02);
        B.setZ(B.z() + 0.02);
        C.setZ(C.z() + 0.02);
        D.setZ(D.z() + 0.02);
        grid.append({ A, B,
                     B, D,
                     A, C,
                     C, D });
    }

    renderImpl->gridRenderImpl_.setColor(QColor(0, 255, 0));
    renderImpl->gridRenderImpl_.setData(grid, GL_LINES);

*/







    // textures to opengl
    for (auto& itmI : globalMesh_.getTileMatrixRef()) {
        for (auto& itmJ : itmI) {
            if (itmJ->getIsUpdate()) {

                // copy to render
                renderImpl->tiles_[itmJ->getUuid()] = *itmJ;

                itmJ->setIsUpdate(false);
            }
        }
    }





    renderImpl->measLinesVertices_.append(std::move(newMeasLinesVertices_));
    renderImpl->measLinesEvenIndices_.append(std::move(newMeasLinesEvenIndices_));
    renderImpl->measLinesOddIndices_.append(std::move(newMeasLinesOddIndices_));



    renderImpl->createBounds();
    lastMatParams_ = actualMatParams;

    Q_EMIT changed();
}

void SideScanView::clear()
{
    auto renderImpl = RENDER_IMPL(SideScanView);
    renderImpl->measLinesVertices_.clear();
    renderImpl->measLinesEvenIndices_.clear();
    renderImpl->measLinesOddIndices_.clear();
    renderImpl->createBounds();

    lastCalcEpoch_ = 0;
    currIndxSec_ = 0;
    lastMatParams_ = MatrixParams();

    isOdds_.clear();
    epochIndxs_.clear();

    // renderImpl->heightVertices_.clear();
    // renderImpl->heightIndices_.clear();
    // renderImpl->textureVertices_.clear();
    // renderImpl->gridRenderImpl_.clearData();

    image_ = QImage();

    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void SideScanView::setScaleFactor(int scaleFactor)
{
    scaleFactor_ = static_cast<float>(scaleFactor);
}

void SideScanView::setMeasLineVisible(bool state)
{
    RENDER_IMPL(SideScanView)->measLineVisible_ = state;

    Q_EMIT changed();
}

void SideScanView::setDatasetPtr(Dataset* datasetPtr)
{
    datasetPtr_ = datasetPtr;
}

void SideScanView::setTextureId(GLuint textureId)
{
   // RENDER_IMPL(SideScanView)->textureId_= textureId;

    Q_EMIT changed();
}

void SideScanView::setGridVisible(bool state)
{
    //RENDER_IMPL(SideScanView)->gridVisible_ = state;

    Q_EMIT changed();
}

QImage& SideScanView::getImagePtr()
{
    return image_;
}

void SideScanView::updateColorTable()
{
    int numColors = 256;
    colorTable_.resize(numColors);
    for (int i = 0; i < numColors; ++i) {
        int grayValue = i * 255 / (numColors - 1);
        colorTable_[i] = qRgb(grayValue, grayValue, grayValue);
    }
}

void SideScanView::updateChannelsIds()
{
    segFChannelId_ = -1;
    segSChannelId_ = -1;

    if (datasetPtr_) {
        if (auto chList = datasetPtr_->channelsList(); chList.size() == 2) {
            auto it = chList.begin();
            segFChannelId_ = it.key();
            segSChannelId_ = (++it).key();
        }
    }
}

MatrixParams SideScanView::getMatrixParams(const QVector<QVector3D> &vertices) const
{
    MatrixParams retVal;

    if (vertices.isEmpty()) {
        return retVal;
    }

    auto [minX, maxX] = std::minmax_element(vertices.begin(), vertices.end(), [](const QVector3D &a, const QVector3D &b) { return a.x() < b.x(); });
    auto [minY, maxY] = std::minmax_element(vertices.begin(), vertices.end(), [](const QVector3D &a, const QVector3D &b) { return a.y() < b.y(); });

    retVal.originX = minX->x();
    //retVal.maxX = maxX->x();
    retVal.originY  = minY->y();
    //retVal.maxY = maxY->y();

    retVal.hMatrixWidth = static_cast<int>(std::ceil(maxX->x() -  minX->x()));
    retVal.imageWidth = retVal.hMatrixWidth;// * globalMesh_.heigtRatio_;


    retVal.hMatrixHeight = static_cast<int>(std::ceil(maxY->y() - minY->y()));
    retVal.imageHeight = retVal.hMatrixHeight;// * globalMesh_.heigtRatio_;

    return retVal;
}

void SideScanView::concatenateMatrixParameters(MatrixParams &srcDst1, const MatrixParams &src2) const
{
    if (!srcDst1.isValid() && !src2.isValid())
        return;

    if (!srcDst1.isValid()) {
        srcDst1 = src2;
        return;
    }

    if (!src2.isValid()) {
        return;
    }


    int maxX = std::max(srcDst1.originX + srcDst1.imageWidth, src2.originX + src2.imageWidth);
    int maxY = std::max(srcDst1.originY + srcDst1.imageHeight, src2.originY + src2.imageHeight);

    srcDst1.originX = std::min(srcDst1.originX, src2.originX);
    //srcDst1.maxX = std::max(srcDst1.maxX, src2.maxX);
    srcDst1.originY = std::min(srcDst1.originY, src2.originY);
    //srcDst1.maxY = std::max(srcDst1.maxY, src2.maxY);


    srcDst1.hMatrixWidth = static_cast<int>(std::ceil(maxX - srcDst1.originX));
    srcDst1.imageWidth = srcDst1.hMatrixWidth;// * globalMesh_.heigtRatio_;

    srcDst1.hMatrixHeight = static_cast<int>(std::ceil(maxY - srcDst1.originY));
    srcDst1.imageHeight = srcDst1.hMatrixHeight;// * globalMesh_.heigtRatio_;
}

int SideScanView::getColorIndx(Epoch::Echogram* charts, int ampIndx) const
{
    if (charts->amplitude.size() > ampIndx) {
        int cVal = charts->amplitude[ampIndx] * (1.5 + ampIndx * 0.0002f);
        cVal = std::min(colorTableSize_, cVal);
        return cVal;
    }
    return 0;
}

bool SideScanView::checkLength(float dist) const
{
    if (qFuzzyCompare(1.0f, 1.0f + dist) || (dist < 0.0f)) {
        return false;
    }
    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// SideScanViewRenderImplementation
SideScanView::SideScanViewRenderImplementation::SideScanViewRenderImplementation() :
    //textureId_(0),
    measLineVisible_(false)//,
    //gridVisible_(false)
{

}

void SideScanView::SideScanViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp,
                                                            const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible) {
        return;
    }

    //if (gridVisible_) {
    //    gridRenderImpl_.render(ctx, mvp, shaderProgramMap);
    //}

    // measure lines
    if (measLineVisible_) {
        // static
        auto staticProgram = shaderProgramMap.value("static", nullptr);

        if (!staticProgram) {
            qWarning() << "Shader program 'static' not found!";
            return;
        }

        staticProgram->bind();
        staticProgram->setUniformValue("mvp", mvp);

        int posLoc = staticProgram->attributeLocation("position");
        staticProgram->enableAttributeArray(posLoc);

        staticProgram->setAttributeArray(posLoc, measLinesVertices_.constData());

        staticProgram->setUniformValue("color", QVector4D(0.0f, 1.0f, 0.0f, 1.0f));
        ctx->glDrawElements(GL_LINES, measLinesEvenIndices_.size(), GL_UNSIGNED_INT, measLinesEvenIndices_.constData());

        staticProgram->setUniformValue("color", QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
        ctx->glDrawElements(GL_LINES, measLinesOddIndices_.size(), GL_UNSIGNED_INT, measLinesOddIndices_.constData());

        staticProgram->disableAttributeArray(posLoc);
        staticProgram->release();
    }


    //qDebug() << "render: " << tiles_.size();

    // surface ща tiles

    auto mosaicProgram = shaderProgramMap.value("mosaic", nullptr);

    if (!mosaicProgram) {
        qWarning() << "Shader program 'mosaic' not found!";
        return;
    }


    mosaicProgram->bind();
    mosaicProgram->setUniformValue("mvp", mvp);

    for (auto& itm : tiles_) {
        int newPosLoc = mosaicProgram->attributeLocation("position");
        int texCoordLoc = mosaicProgram->attributeLocation("texCoord");

        mosaicProgram->enableAttributeArray(newPosLoc);
        mosaicProgram->enableAttributeArray(texCoordLoc);

        mosaicProgram->setAttributeArray(newPosLoc , itm.getHeightVerticesRef().constData());
        mosaicProgram->setAttributeArray(texCoordLoc, itm.getTextureVerticesRef().constData());


        if (itm.getTextureId()) {
            glBindTexture(GL_TEXTURE_2D, itm.getTextureId());
        }

        ctx->glDrawElements(GL_TRIANGLES, itm.getHeightIndicesRef().size(), GL_UNSIGNED_INT, itm.getHeightIndicesRef().constData());


        mosaicProgram->disableAttributeArray(texCoordLoc);
        mosaicProgram->disableAttributeArray(newPosLoc);
    }

    mosaicProgram->release();

}

void SideScanView::SideScanViewRenderImplementation::createBounds()
{
    if (measLinesVertices_.isEmpty()) {
        m_bounds = Cube();
        return;
    }

    float z_max{ !std::isfinite(measLinesVertices_.first().z()) ? 0.f : measLinesVertices_.first().z() };
    float z_min{ z_max };
    float x_max{ !std::isfinite(measLinesVertices_.first().x()) ? 0.f : measLinesVertices_.first().x() };
    float x_min{ x_max };
    float y_max{ !std::isfinite(measLinesVertices_.first().y()) ? 0.f : measLinesVertices_.first().y() };
    float y_min{ y_max };

    for (const auto& itm: qAsConst(measLinesVertices_)){
        z_min = std::min(z_min, !std::isfinite(itm.z()) ? 0.f : itm.z());
        z_max = std::max(z_max, !std::isfinite(itm.z()) ? 0.f : itm.z());
        x_min = std::min(x_min, !std::isfinite(itm.x()) ? 0.f : itm.x());
        x_max = std::max(x_max, !std::isfinite(itm.x()) ? 0.f : itm.x());
        y_min = std::min(y_min, !std::isfinite(itm.y()) ? 0.f : itm.y());
        y_max = std::max(y_max, !std::isfinite(itm.y()) ? 0.f : itm.y());
    }

    m_bounds = Cube(x_min, x_max, y_min, y_max, z_min, z_max);
}

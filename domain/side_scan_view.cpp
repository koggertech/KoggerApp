#include "side_scan_view.h"

#include <QtMath>
#include "graphicsscene3dview.h"


SideScanView::SideScanView(QObject* parent) :
    SceneObject(new SideScanViewRenderImplementation, parent),
    datasetPtr_(nullptr),
    scaleFactor_(1.0f),
    segFChannelId_(-1),
    segSChannelId_(-1)
{
    updateColorTable();
}

SideScanView::~SideScanView()
{

}

void SideScanView::updateData()
{
    if (!datasetPtr_) {
        qDebug() << "SideScanView::updateDataSec: dataset is nullptr!";
        return;
    }

    auto renderImpl = RENDER_IMPL(SideScanView);
    updateChannelsIds();

    auto epochCount = datasetPtr_->size();
    if (epochCount < 4)
        return;


    // prepare intermediate data
    MatrixParams actualMatParams(lastMatParams_);
    MatrixParams newMatrixParams;

    QVector<QVector3D> measLinesVertices_;
    QVector<int> measLinesEvenIndices_;
    QVector<int> measLinesOddIndices_;

    QVector<char> isOdds;
    QVector<int> epochIndxs;

    for (int i = lastAcceptedEpoch_; i < epochCount; ++i) {
        bool isAcceptedEpoch = false;

        if (auto epoch = datasetPtr_->fromIndex(i); epoch) {
            auto pos = epoch->getPositionGNSS().ned;
            auto yaw = epoch->yaw();
            if (isfinite(pos.n) && isfinite(pos.e) && isfinite(yaw)) {
                bool acceptedEven = false, acceptedOdd = false;
                double azRad = qDegreesToRadians(yaw);
                if (auto segFCharts = epoch->chart(segFChannelId_); segFCharts) {
                    double leftAzRad = azRad - M_PI_2;
                    float lDist = segFCharts->range();

                    measLinesVertices_.append(QVector3D(pos.n + lDist * qCos(leftAzRad), pos.e + lDist * qSin(leftAzRad), 0.0f));
                    measLinesVertices_.append(QVector3D(pos.n, pos.e, 0.0f));
                    measLinesEvenIndices_.append(currIndxSec_++);
                    measLinesEvenIndices_.append(currIndxSec_++);

                    isOdds.append('0');
                    epochIndxs.append(i);

                    acceptedEven = true;
                }
                if (auto segSCharts = epoch->chart(segSChannelId_); segSCharts) {
                    double rightAzRad = azRad + M_PI_2;
                    float rDist = segSCharts ->range();

                    measLinesVertices_.append(QVector3D(pos.n, pos.e, 0.0f));
                    measLinesVertices_.append(QVector3D(pos.n + rDist * qCos(rightAzRad), pos.e + rDist * qSin(rightAzRad), 0.0f));
                    measLinesOddIndices_.append(currIndxSec_++);
                    measLinesOddIndices_.append(currIndxSec_++);

                    isOdds.append('1');
                    epochIndxs.append(i);

                    acceptedOdd = true;
                }

                if (acceptedEven && acceptedOdd) {
                    isAcceptedEpoch = true;
                }
            }
        }

        if (isAcceptedEpoch) {
            lastAcceptedEpoch_ = i;
        }
    }

    lastCalcEpoch_ = epochCount;

    newMatrixParams = getMatrixParams(measLinesVertices_);

    if (!newMatrixParams.isValid()) {
        return;
    }

    concatenateMatrixParameters(actualMatParams, newMatrixParams);

    bool meshUpdated = globalMesh_.concatenate(actualMatParams);

    if (meshUpdated) {
        // // just debug messages
        // qDebug() << "/// inserted start ///";
        // qDebug() << "actual matrix:";
        // actualMatParams.print(qDebug());
        // qDebug() << "globalmesh :";
        // globalMesh_.printMatrix();
        // qDebug() << "/// inserted end ///";
    }


    // processing
    for (int i = 0; i < measLinesVertices_.size(); i += 2) { // 2 - step for segment
        if (i + 8 >= measLinesVertices_.size()) {
            break;
        }

        int segFBegVertIndx = i;
        int segFEndVertIndx = i + 1;
        int segSBegVertIndx = i + 2;
        int segSEndVertIndx = i + 3;

        int segFIndx = i / 2;
        int segSIndx = (i + 2) / 2;
        // going to next epoch if needed
        if (epochIndxs[segFIndx] == epochIndxs[segSIndx] ||
            isOdds[segFIndx] != isOdds[segSIndx]) {
            ++segSIndx;
            segSBegVertIndx += 2;
            segSEndVertIndx += 2;
        }
        // compliance check
        if (epochIndxs[segFIndx] == epochIndxs[segSIndx] ||
            isOdds[segFIndx] != isOdds[segSIndx]) {
            continue;
        }
        // epochs checking
        auto segFEpochPtr = datasetPtr_->fromIndex(epochIndxs[segFIndx]); // _plot might be reallocated
        auto segSEpochPtr = datasetPtr_->fromIndex(epochIndxs[segSIndx]);
        if (!segFEpochPtr || !segSEpochPtr) {
            continue;
        }
        Epoch segFEpoch = *segFEpochPtr;
        Epoch segSEpoch = *segFEpochPtr;
        // isOdd checking
        bool segFIsOdd = isOdds[segFIndx] == '1';
        bool segSIsOdd = isOdds[segSIndx] == '1';
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


        auto globalMeshOrigin = globalMesh_.getOrigin();
        auto globalWidth = globalMesh_.getPixelWidth();
        auto globalHeight = globalMesh_.getPixelHeight();


        // Bresenham
        // first segment
        QVector3D segFBegPoint = !segFIsOdd ? measLinesVertices_[segFBegVertIndx] : measLinesVertices_[segFEndVertIndx];
        QVector3D segSEngPoint = !segFIsOdd ? measLinesVertices_[segFEndVertIndx] : measLinesVertices_[segFBegVertIndx];
        int segFX1 = std::min(globalWidth - 1,  std::max(0, static_cast<int>(std::round((segFBegPoint.x() - globalMeshOrigin.x()) * scaleFactor_))));
        int segFY1 = std::min(globalHeight - 1, std::max(0, static_cast<int>(std::round((segFBegPoint.y() - globalMeshOrigin.y()) * scaleFactor_))));
        int segFX2 = std::min(globalWidth - 1,  std::max(0, static_cast<int>(std::round((segSEngPoint.x() - globalMeshOrigin.x()) * scaleFactor_))));
        int segFY2 = std::min(globalHeight - 1, std::max(0, static_cast<int>(std::round((segSEngPoint.y() - globalMeshOrigin.y()) * scaleFactor_))));
        float segFPixTotDist = std::sqrt(std::pow(segFX2 - segFX1, 2) + std::pow(segFY2 - segFY1, 2));
        int segFDx = std::abs(segFX2 - segFX1);
        int segFDy = std::abs(segFY2 - segFY1);
        int segFSx = (segFX1 < segFX2) ? 1 : -1;
        int segFSy = (segFY1 < segFY2) ? 1 : -1;
        int segFErr = segFDx - segFDy;
        // second segment
        QVector3D segSBegPoint = !segSIsOdd ? measLinesVertices_[segSBegVertIndx] : measLinesVertices_[segSEndVertIndx];
        QVector3D segSEndPoint = !segSIsOdd ? measLinesVertices_[segSEndVertIndx] : measLinesVertices_[segSBegVertIndx];
        int segSX1 = std::min(globalWidth - 1,  std::max(0, static_cast<int>(std::round((segSBegPoint.x() - globalMeshOrigin.x()) * scaleFactor_))));
        int segSY1 = std::min(globalHeight - 1, std::max(0, static_cast<int>(std::round((segSBegPoint.y() - globalMeshOrigin.y()) * scaleFactor_))));
        int segSX2 = std::min(globalWidth - 1,  std::max(0, static_cast<int>(std::round((segSEndPoint.x() - globalMeshOrigin.x()) * scaleFactor_))));
        int segSY2 = std::min(globalHeight - 1, std::max(0, static_cast<int>(std::round((segSEndPoint.y() - globalMeshOrigin.y()) * scaleFactor_))));
        float segSPixTotDist = std::sqrt(std::pow(segSX2 - segSX1, 2) + std::pow(segSY2 - segSY1, 2));
        int segSDx = std::abs(segSX2 - segSX1);
        int segSDy = std::abs(segSY2 - segSY1);
        int segSSx = (segSX1 < segSX2) ? 1 : -1;
        int segSSy = (segSY1 < segSY2) ? 1 : -1;
        int segSErr = segSDx - segSDy;

        // pixel length checking
        if (!checkLength(segFPixTotDist) ||
            !checkLength(segSPixTotDist)) {
            continue;
        }

        float segFDistProc = -1.0f * static_cast<float>(segFEpoch.distProccesing(segFIsOdd ? segFChannelId_ : segSChannelId_));
        float segSDistProc = -1.0f * static_cast<float>(segSEpoch.distProccesing(segSIsOdd ? segFChannelId_ : segSChannelId_));
        float segFDistX = segSEngPoint.x() - segFBegPoint.x();
        float segFDistY = segSEngPoint.y() - segFBegPoint.y();
        float segSDistX = segSEndPoint.x() - segSBegPoint.x();
        float segSDistY = segSEndPoint.y() - segSBegPoint.y();



        // follow the first segment
        while (true) {
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
            int interpX1 = std::min(globalWidth - 1,  std::max(0, static_cast<int>(std::round((segFPixPos.x() - globalMeshOrigin.x()) * scaleFactor_))));
            int interpY1 = std::min(globalHeight - 1, std::max(0, static_cast<int>(std::round((segFPixPos.y() - globalMeshOrigin.y()) * scaleFactor_))));
            int interpX2 = std::min(globalWidth - 1,  std::max(0, static_cast<int>(std::round((segSPixPos.x() - globalMeshOrigin.x()) * scaleFactor_))));
            int interpY2 = std::min(globalHeight - 1, std::max(0, static_cast<int>(std::round((segSPixPos.y() - globalMeshOrigin.y()) * scaleFactor_))));
            int interpDistX = interpX2 - interpX1;
            int interpDistY = interpY2 - interpY1;
            float interpPixTotDist = std::sqrt(std::pow(interpX2 - interpX1, 2) + std::pow(interpY2 - interpY1, 2));


            //// height matrix
            //int uinterpX1 = std::min(globalWidth  - 1, std::max(0, static_cast<int>(std::round((segFPixPos.x() - globalMeshOrigin.x())))));
            //int uinterpY1 = std::min(globalHeight - 1, std::max(0, static_cast<int>(std::round((segFPixPos.y() - globalMeshOrigin.y())))));
            //int uinterpX2 = std::min(globalWidth  - 1, std::max(0, static_cast<int>(std::round((segSPixPos.x() - globalMeshOrigin.x())))));
            //int uinterpY2 = std::min(globalHeight - 1, std::max(0, static_cast<int>(std::round((segSPixPos.y() - globalMeshOrigin.y())))));
            //int uinterpDistX = uinterpX2 - uinterpX1;
            //int uinterpDistY = uinterpY2 - uinterpY1;



            //// meas line
            //int meshIndxX = segFX1 / globalMesh_.getTileSize();
            //int meshIndxY = (globalMesh_.getNumHeightTiles() - 1) - segFY1 / globalMesh_.getTileSize();
            //int tileIndxY = segFY1 % globalMesh_.getTileSize();
            //int tileIndxX = segFX1 % globalMesh_.getTileSize();
            //auto& imageRef = globalMesh_.getTileMatrixRef()[meshIndxY][meshIndxX]->getImageRef();
            //uchar* imageData = imageRef.bits();
            //int bytesPerLine = imageRef.bytesPerLine();
            //int bytesInPix = bytesPerLine / imageRef.width();
            //uchar* pixPtr = imageData + tileIndxY * bytesPerLine + tileIndxX * bytesInPix;
            //*pixPtr = colorTable_[segFColorIndx];
            //globalMesh_.getTileMatrixRef()[meshIndxY][meshIndxX]->setIsUpdate(true);



            // interpolate
            if (checkLength(interpPixTotDist)) {
                for (int step = 0; step <= interpPixTotDist; ++step) {
                    float interpProgress = static_cast<float>(step) / interpPixTotDist;
                    int interpX = interpX1 + interpProgress * interpDistX;
                    int interpY = interpY1 + interpProgress * interpDistY;
                    auto interpColorIndx = static_cast<int>((1 - interpProgress) * segFColorIndx + interpProgress * segSColorIndx);


                    //// height matrix
                    //int uinterpX = uinterpX1 + interpProgress * uinterpDistX;
                    //int uinterpY = uinterpY1 + interpProgress * uinterpDistY;

                    for (int offsetX = -interpLineWidth_; offsetX <= interpLineWidth_; ++offsetX) {
                        for (int offsetY = -interpLineWidth_; offsetY <= interpLineWidth_; ++offsetY) {

                            int applyInterpX = std::min(globalWidth - 1, std::max(0, interpX + offsetX));
                            int applyInterpY = std::min(globalHeight - 1, std::max(0, interpY + offsetY));



                            int meshIndxX = applyInterpX / globalMesh_.getTileSize();
                            int meshIndxY = (globalMesh_.getNumHeightTiles() - 1) - applyInterpY / globalMesh_.getTileSize();
                            int tileIndxX = applyInterpX % globalMesh_.getTileSize();
                            int tileIndxY = applyInterpY % globalMesh_.getTileSize();


                            auto& imageRef = globalMesh_.getTileMatrixRef()[meshIndxY][meshIndxX]->getImageRef();
                            uchar* imageData = imageRef.bits();
                            int bytesPerLine = imageRef.bytesPerLine();
                            int bytesInPix = bytesPerLine / imageRef.width();

                            uchar* pixPtr = imageData + tileIndxY * bytesPerLine + tileIndxX * bytesInPix;
                            *pixPtr = colorTable_[interpColorIndx];


                            globalMesh_.getTileMatrixRef()[meshIndxY][meshIndxX]->setIsUpdate(true);


                            //// height matrix
                            //int hStepsInRow = globalMesh_.getTileSize() / globalMesh_.getHeightStep();
                            ////// height matrix
                            //int uApplyInterpX = std::min(globalMesh_.getTileSize() - 1, std::max(0, tileIndxX + offsetX));
                            //int uApplyInterpY = std::min(globalMesh_.getTileSize() - 1, std::max(0, tileIndxY + offsetY));
                            //int heightVerticesIndx = (uApplyInterpY / globalMesh_.getHeightStep()) * hStepsInRow + (uApplyInterpX / globalMesh_.getHeightStep());
                            //globalMesh_.getTileMatrixRef()[meshIndxY][meshIndxX]->getHeightVerticesRef()[heightVerticesIndx][2] = segSPixPos.z();
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


    // texture update
    for (auto& itmI : globalMesh_.getTileMatrixRef()) {
        for (auto& itmJ : itmI) {
            if (itmJ->getIsUpdate()) {
                renderImpl->tiles_[itmJ->getUuid()] = *itmJ; // copy data to render

                if (m_view) {
                    m_view->updateTileTexture(itmJ->getUuid(), itmJ->getImageRef());
                }

                itmJ->setIsUpdate(false);
            }
        }
    }


    renderImpl->measLinesVertices_.append(std::move(measLinesVertices_));
    renderImpl->measLinesEvenIndices_.append(std::move(measLinesEvenIndices_));
    renderImpl->measLinesOddIndices_.append(std::move(measLinesOddIndices_));

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
    lastAcceptedEpoch_ = 0;
    currIndxSec_ = 0;
    lastMatParams_ = MatrixParams();

    globalMesh_.clear();

    origin_ = QVector3D();

    renderImpl->tiles_.clear();

    for (const auto &itmI : globalMesh_.getTileMatrixRef()) {
        for (const auto& itmJ : itmI) {

            if (m_view) {
                m_view->updateTileTexture(itmJ->getUuid(), QImage());
            }
        }
    }

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

void SideScanView::setTextureIdForTile(QUuid tileid, GLuint textureId)
{
    RENDER_IMPL(SideScanView)->tiles_[tileid].setTextureId(textureId);

    Q_EMIT changed();
}

void SideScanView::setTileGridVisible(bool state)
{
    RENDER_IMPL(SideScanView)->tileGridVisible_ = state;

    Q_EMIT changed();
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

void SideScanView::setView(GraphicsScene3dView *viewPtr)
{
    SceneObject::m_view = viewPtr;
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
    retVal.originY  = minY->y();

    retVal.rawWidth = static_cast<int>(std::ceil(maxX->x() -  minX->x()));
    retVal.heightMatrixWidth = retVal.rawWidth;
    retVal.imageWidth        = retVal.rawWidth * scaleFactor_;

    retVal.rawHeight = static_cast<int>(std::ceil(maxY->y() - minY->y()));
    retVal.heightMatrixHeight = retVal.rawHeight;
    retVal.imageHeight        = retVal.rawHeight * scaleFactor_;

    return retVal;
}

void SideScanView::concatenateMatrixParameters(MatrixParams &srcDst, const MatrixParams &src) const
{
    if (!srcDst.isValid() && !src.isValid())
        return;

    if (!srcDst.isValid()) {
        srcDst = src;
        return;
    }

    if (!src.isValid()) {
        return;
    }

    int maxX = std::max(srcDst.originX + srcDst.rawWidth, src.originX + src.rawWidth);
    int maxY = std::max(srcDst.originY + srcDst.rawHeight, src.originY + src.rawHeight);

    srcDst.originX = std::min(srcDst.originX, src.originX);
    srcDst.originY = std::min(srcDst.originY, src.originY);

    srcDst.rawWidth = static_cast<int>(std::ceil(maxX - srcDst.originX));
    srcDst.heightMatrixWidth = srcDst.rawWidth;
    srcDst.imageWidth = srcDst.rawWidth + scaleFactor_;

    srcDst.rawHeight = static_cast<int>(std::ceil(maxY - srcDst.originY));
    srcDst.heightMatrixHeight = srcDst.rawHeight;
    srcDst.imageHeight = srcDst.rawHeight * scaleFactor_;
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
    measLineVisible_(true),
    tileGridVisible_(false)
{

}

void SideScanView::SideScanViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp,
                                                            const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible) {
        return;
    }

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


    auto mosaicProgram = shaderProgramMap.value("mosaic", nullptr);

    if (!mosaicProgram) {
        qWarning() << "Shader program 'mosaic' not found!";
        return;
    }




    for (auto& itm : tiles_) {

        if (tileGridVisible_) {
            itm.getGridRenderImplRef().render(ctx, mvp, shaderProgramMap);
        }


        mosaicProgram->bind();
        mosaicProgram->setUniformValue("mvp", mvp);


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

        mosaicProgram->release();
    }


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

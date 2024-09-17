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
    // TODO:
    //  refactor (naming etc.)
    //  perfomance optimizations
    //  check epoch selection logic
    //  height matrix


    if (!datasetPtr_) {
        qDebug() << "SideScanView::updateDataSec: dataset is nullptr!";
        return;
    }

    auto renderImpl = RENDER_IMPL(SideScanView);
    updateChannelsIds();

    auto epochCount = datasetPtr_->size();
    if (epochCount < 4)
        return;


    // prepare intermediate data (selecting epochs to process)
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


        //auto globalMeshOrigin = globalMesh_.getOrigin();
        auto globalWidthPixels = globalMesh_.getWidthPixels();
        auto globalHeightPixels = globalMesh_.getHeightPixels();


        // Bresenham
        // first segment
        QVector3D segFPhysicsBegPoint = !segFIsOdd ? measLinesVertices_[segFBegVertIndx] : measLinesVertices_[segFEndVertIndx]; // physics coordinates
        QVector3D segFPhysicsEndPoint = !segFIsOdd ? measLinesVertices_[segFEndVertIndx] : measLinesVertices_[segFBegVertIndx];
        auto segFBegPixelPos = globalMesh_.convertPhysicsCoordinateToPixel(segFPhysicsBegPoint);
        auto segFEndPixelPos = globalMesh_.convertPhysicsCoordinateToPixel(segFPhysicsEndPoint);
        int segFPixelX1 = std::min(globalWidthPixels - 1,  std::max(0, static_cast<int>(std::round(segFBegPixelPos.x()))));
        int segFPixelY1 = std::min(globalHeightPixels - 1, std::max(0, static_cast<int>(std::round(segFBegPixelPos.y()))));
        int segFPixelX2 = std::min(globalWidthPixels - 1,  std::max(0, static_cast<int>(std::round(segFEndPixelPos.x()))));
        int segFPixelY2 = std::min(globalHeightPixels - 1, std::max(0, static_cast<int>(std::round(segFEndPixelPos.y()))));
        float segFPixelTotDist = std::sqrt(std::pow(segFPixelX2 - segFPixelX1, 2) + std::pow(segFPixelY2 - segFPixelY1, 2));
        int segFPixelDx = std::abs(segFPixelX2 - segFPixelX1);
        int segFPixelDy = std::abs(segFPixelY2 - segFPixelY1);
        int segFPixelSx = (segFPixelX1 < segFPixelX2) ? 1 : -1;
        int segFPixelSy = (segFPixelY1 < segFPixelY2) ? 1 : -1;
        int segFPixelErr = segFPixelDx - segFPixelDy;

        // second segment
        QVector3D segSPhysicsBegPoint = !segSIsOdd ? measLinesVertices_[segSBegVertIndx] : measLinesVertices_[segSEndVertIndx];
        QVector3D segSPhysicsEndPoint = !segSIsOdd ? measLinesVertices_[segSEndVertIndx] : measLinesVertices_[segSBegVertIndx];
        auto segSBegPixelPos = globalMesh_.convertPhysicsCoordinateToPixel(segSPhysicsBegPoint);
        auto segSEndPixelPos = globalMesh_.convertPhysicsCoordinateToPixel(segSPhysicsEndPoint);
        int segSPixelX1 = std::min(globalWidthPixels - 1,  std::max(0, static_cast<int>(std::round(segSBegPixelPos.x()))));
        int segSPixelY1 = std::min(globalHeightPixels - 1, std::max(0, static_cast<int>(std::round(segSBegPixelPos.y()))));
        int segSPixelX2 = std::min(globalWidthPixels - 1,  std::max(0, static_cast<int>(std::round(segSEndPixelPos.x()))));
        int segSPixelY2 = std::min(globalHeightPixels - 1, std::max(0, static_cast<int>(std::round(segSEndPixelPos.y()))));
        float segSPixelTotDist = std::sqrt(std::pow(segSPixelX2 - segSPixelX1, 2) + std::pow(segSPixelY2 - segSPixelY1, 2));
        int segSPixelDx = std::abs(segSPixelX2 - segSPixelX1);
        int segSPixelDy = std::abs(segSPixelY2 - segSPixelY1);
        int segSPixelSx = (segSPixelX1 < segSPixelX2) ? 1 : -1;
        int segSPixelSy = (segSPixelY1 < segSPixelY2) ? 1 : -1;
        int segSPixelErr = segSPixelDx - segSPixelDy;

        // pixel length checking
        if (!checkLength(segFPixelTotDist) ||
            !checkLength(segSPixelTotDist)) {
            qDebug() << "pixel length checking: YOU SHALL NOT PASS!";
            continue;
        }

        float segFDistProc = -1.0f * static_cast<float>(segFEpoch.distProccesing(segFIsOdd ? segFChannelId_ : segSChannelId_));
        float segSDistProc = -1.0f * static_cast<float>(segSEpoch.distProccesing(segSIsOdd ? segFChannelId_ : segSChannelId_));
        float segFMetersDistX = segFPhysicsEndPoint.x() - segFPhysicsBegPoint.x();
        float segFMetersDistY = segFPhysicsEndPoint.y() - segFPhysicsBegPoint.y();
        float segSMetersDistX = segSPhysicsEndPoint.x() - segSPhysicsBegPoint.x();
        float segSMetersDistY = segSPhysicsEndPoint.y() - segSPhysicsBegPoint.y();

        // follow the first segment
        while (true) {
            // first segment
            float segFPixelCurrDist = std::sqrt(std::pow(segFPixelX1 - segFPixelX2, 2) + std::pow(segFPixelY1 - segFPixelY2, 2));
            float segFProgressByPixels = std::min(1.0f, segFPixelCurrDist / segFPixelTotDist);
            QVector3D segFCurrPhysicsPos(segFPhysicsBegPoint.x() + segFProgressByPixels * segFMetersDistX, segFPhysicsBegPoint.y() + segFProgressByPixels * segFMetersDistY, segFDistProc);
            QVector3D segFBoatPos(segFEpoch.getPositionGNSS().ned.n, segFEpoch.getPositionGNSS().ned.e, 0.0f);
            auto segFColorIndx = getColorIndx(segFCharts, static_cast<int>(std::floor(segFCurrPhysicsPos.distanceToPoint(segFBoatPos) * amplitudeCoeff_)));
            // second segment, calc corresponding progress using smoothed interpolation
            float segSCorrProgressByPixels = std::min(1.0f, segFPixelCurrDist / segFPixelTotDist * segSPixelTotDist / segFPixelTotDist);
            QVector3D segSCurrPhysicsPos(segSPhysicsBegPoint.x() + segSCorrProgressByPixels * segSMetersDistX,    segSPhysicsBegPoint.y() + segSCorrProgressByPixels * segSMetersDistY,     segSDistProc);
            QVector3D segSBoatPos(segSEpoch.getPositionGNSS().ned.n, segSEpoch.getPositionGNSS().ned.e, 0.0f);
            auto segSColorIndx  = getColorIndx(segSCharts, static_cast<int>(std::floor(segSCurrPhysicsPos.distanceToPoint(segSBoatPos) * amplitudeCoeff_)));

            auto segFCurrPixelPos = globalMesh_.convertPhysicsCoordinateToPixel(segFCurrPhysicsPos);
            auto segSCurrPixelPos = globalMesh_.convertPhysicsCoordinateToPixel(segSCurrPhysicsPos);

            // color interpolation between two pixels
            int interpPixelX1 = std::min(globalWidthPixels - 1,  std::max(0, static_cast<int>(std::round(segFCurrPixelPos.x()))));
            int interpPixelY1 = std::min(globalHeightPixels - 1, std::max(0, static_cast<int>(std::round(segFCurrPixelPos.y()))));
            int interpPixelX2 = std::min(globalWidthPixels - 1,  std::max(0, static_cast<int>(std::round(segSCurrPixelPos.x()))));
            int interpPixelY2 = std::min(globalHeightPixels - 1, std::max(0, static_cast<int>(std::round(segSCurrPixelPos.y()))));

            int interpPixelDistX = interpPixelX2 - interpPixelX1;
            int interpPixelDistY = interpPixelY2 - interpPixelY1;
            float interpPixelTotDist = std::sqrt(std::pow(interpPixelDistX, 2) + std::pow(interpPixelDistY, 2));


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
            if (checkLength(interpPixelTotDist)) {
                for (int step = 0; step <= interpPixelTotDist; ++step) {
                    float interpProgressByPixel = static_cast<float>(step) / interpPixelTotDist;
                    int interpX = interpPixelX1 + interpProgressByPixel * interpPixelDistX;
                    int interpY = interpPixelY1 + interpProgressByPixel * interpPixelDistY;
                    auto interpColorIndx = static_cast<int>((1 - interpProgressByPixel) * segFColorIndx + interpProgressByPixel * segSColorIndx);

                    //// height matrix
                    //int uinterpX = uinterpX1 + interpProgress * uinterpDistX;
                    //int uinterpY = uinterpY1 + interpProgress * uinterpDistY;

                    for (int offsetX = -interpLineWidth_; offsetX <= interpLineWidth_; ++offsetX) {
                        for (int offsetY = -interpLineWidth_; offsetY <= interpLineWidth_; ++offsetY) {

                            int applyInterpX = std::min(globalWidthPixels - 1, std::max(0, interpX + offsetX));
                            int applyInterpY = std::min(globalHeightPixels - 1, std::max(0, interpY + offsetY));

                            int meshIndxX = applyInterpX / globalMesh_.getTilePixelSize();
                            int meshIndxY = (globalMesh_.getNumHeightTiles() - 1) - applyInterpY / globalMesh_.getTilePixelSize();
                            int tileIndxX = applyInterpX % globalMesh_.getTilePixelSize();
                            int tileIndxY = applyInterpY % globalMesh_.getTilePixelSize();

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
            if (segFPixelX1 == segFPixelX2 && segFPixelY1 == segFPixelY2) {
                break;
            }

            // Bresenham
            int segFPixelE2 = 2 * segFPixelErr;
            if (segFPixelE2 > -segFPixelDy) {
                segFPixelErr -= segFPixelDy;
                segFPixelX1 += segFPixelSx;
            }
            if (segFPixelE2 < segFPixelDx) {
                segFPixelErr += segFPixelDx;
                segFPixelY1 += segFPixelSy;
            }
            int segSPixelE2 = 2 * segSPixelErr;
            if (segSPixelE2 > -segSPixelDy) {
                segSPixelErr -= segSPixelDy;
                segSPixelX1 += segSPixelSx;
            }
            if (segSPixelE2 < segSPixelDx) {
                segSPixelErr += segSPixelDx;
                segSPixelY1 += segSPixelSy;
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

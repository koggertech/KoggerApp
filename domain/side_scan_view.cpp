#include "side_scan_view.h"

#include <QtMath>
#include <QImage>

#include "graphicsscene3dview.h"


SideScanView::SideScanView(QObject* parent) :
    SceneObject(new SideScanViewRenderImplementation, parent),
    datasetPtr_(nullptr),
    tileResolution_(0.1), // 0.1 метр - 1 пиксель
    currIndxSec_(0),
    segFChannelId_(-1),
    segSChannelId_(-1),
    tileSidePixelSize_(256),
    tileHeightMatrixRatio_(16),
    lastCalcEpoch_(0),
    lastAcceptedEpoch_(0),
    globalMesh_(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_)
{
    updateColorTable();
}

SideScanView::~SideScanView()
{

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

    updateChannelsIds(); // cause we dont know when

    auto epochCount = datasetPtr_->size();
    if (epochCount < 4) {
        return;
    }

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
        auto segFEpochPtr = datasetPtr_->fromIndex(epochIndxs[segFIndx]);
        auto segSEpochPtr = datasetPtr_->fromIndex(epochIndxs[segSIndx]);
        if (!segFEpochPtr || !segSEpochPtr) {
            continue;
        }
        Epoch segFEpoch = *segFEpochPtr; // _plot might be reallocated
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

        auto gMeshWidthPixs = globalMesh_.getPixelWidth();
        auto gMeshHeightPixs = globalMesh_.getPixelHeight();

        // Bresenham
        // first segment
        QVector3D segFPhBegPnt = !segFIsOdd ? measLinesVertices_[segFBegVertIndx] : measLinesVertices_[segFEndVertIndx]; // physics coordinates
        QVector3D segFPhEndPnt = !segFIsOdd ? measLinesVertices_[segFEndVertIndx] : measLinesVertices_[segFBegVertIndx];
        auto segFBegPixPos = globalMesh_.convertPhToPixCoords(segFPhBegPnt);
        auto segFEndPixPos = globalMesh_.convertPhToPixCoords(segFPhEndPnt);
        int segFPixX1 = std::min(gMeshWidthPixs - 1,  std::max(0, static_cast<int>(std::round(segFBegPixPos.x()))));
        int segFPixY1 = std::min(gMeshHeightPixs - 1, std::max(0, static_cast<int>(std::round(segFBegPixPos.y()))));
        int segFPixX2 = std::min(gMeshWidthPixs - 1,  std::max(0, static_cast<int>(std::round(segFEndPixPos.x()))));
        int segFPixY2 = std::min(gMeshHeightPixs - 1, std::max(0, static_cast<int>(std::round(segFEndPixPos.y()))));
        float segFPixTotDist = std::sqrt(std::pow(segFPixX2 - segFPixX1, 2) + std::pow(segFPixY2 - segFPixY1, 2));
        int segFPixDx = std::abs(segFPixX2 - segFPixX1);
        int segFPixDy = std::abs(segFPixY2 - segFPixY1);
        int segFPixSx = (segFPixX1 < segFPixX2) ? 1 : -1;
        int segFPixSy = (segFPixY1 < segFPixY2) ? 1 : -1;
        int segFPixErr = segFPixDx - segFPixDy;
        // second segment
        QVector3D segSPhBegPnt = !segSIsOdd ? measLinesVertices_[segSBegVertIndx] : measLinesVertices_[segSEndVertIndx];
        QVector3D segSPhEndPnt = !segSIsOdd ? measLinesVertices_[segSEndVertIndx] : measLinesVertices_[segSBegVertIndx];
        auto segSBegPixPos = globalMesh_.convertPhToPixCoords(segSPhBegPnt);
        auto segSEndPixPos = globalMesh_.convertPhToPixCoords(segSPhEndPnt);
        int segSPixX1 = std::min(gMeshWidthPixs - 1,  std::max(0, static_cast<int>(std::round(segSBegPixPos.x()))));
        int segSPixY1 = std::min(gMeshHeightPixs - 1, std::max(0, static_cast<int>(std::round(segSBegPixPos.y()))));
        int segSPixX2 = std::min(gMeshWidthPixs - 1,  std::max(0, static_cast<int>(std::round(segSEndPixPos.x()))));
        int segSPixY2 = std::min(gMeshHeightPixs - 1, std::max(0, static_cast<int>(std::round(segSEndPixPos.y()))));
        float segSPixTotDist = std::sqrt(std::pow(segSPixX2 - segSPixX1, 2) + std::pow(segSPixY2 - segSPixY1, 2));
        int segSPixDx = std::abs(segSPixX2 - segSPixX1);
        int segSPixDy = std::abs(segSPixY2 - segSPixY1);
        int segSPixSx = (segSPixX1 < segSPixX2) ? 1 : -1;
        int segSPixSy = (segSPixY1 < segSPixY2) ? 1 : -1;
        int segSPixErr = segSPixDx - segSPixDy;

        // pixel length checking
        if (!checkLength(segFPixTotDist) ||
            !checkLength(segSPixTotDist)) {
            continue;
        }

        float segFDistProc = -1.0f * static_cast<float>(segFEpoch.distProccesing(segFIsOdd ? segFChannelId_ : segSChannelId_));
        float segSDistProc = -1.0f * static_cast<float>(segSEpoch.distProccesing(segSIsOdd ? segFChannelId_ : segSChannelId_));
        float segFPhDistX = segFPhEndPnt.x() - segFPhBegPnt.x();
        float segFPhDistY = segFPhEndPnt.y() - segFPhBegPnt.y();
        float segSPhDistX = segSPhEndPnt.x() - segSPhBegPnt.x();
        float segSPhDistY = segSPhEndPnt.y() - segSPhBegPnt.y();

        // follow the first segment
        while (true) {
            // first segment
            float segFPixCurrDist = std::sqrt(std::pow(segFPixX1 - segFPixX2, 2) + std::pow(segFPixY1 - segFPixY2, 2));
            float segFProgByPix = std::min(1.0f, segFPixCurrDist / segFPixTotDist);
            QVector3D segFCurrPhPos(segFPhBegPnt.x() + segFProgByPix * segFPhDistX, segFPhBegPnt.y() + segFProgByPix * segFPhDistY, segFDistProc);
            QVector3D segFBoatPos(segFEpoch.getPositionGNSS().ned.n, segFEpoch.getPositionGNSS().ned.e, 0.0f);
            auto segFColorIndx = getColorIndx(segFCharts, static_cast<int>(std::floor(segFCurrPhPos.distanceToPoint(segFBoatPos) * amplitudeCoeff_)));
            // second segment, calc corresponding progress using smoothed interpolation
            float segSCorrProgByPix = std::min(1.0f, segFPixCurrDist / segFPixTotDist * segSPixTotDist / segFPixTotDist);
            QVector3D segSCurrPhPos(segSPhBegPnt.x() + segSCorrProgByPix * segSPhDistX,    segSPhBegPnt.y() + segSCorrProgByPix * segSPhDistY,     segSDistProc);
            QVector3D segSBoatPos(segSEpoch.getPositionGNSS().ned.n, segSEpoch.getPositionGNSS().ned.e, 0.0f);
            auto segSColorIndx  = getColorIndx(segSCharts, static_cast<int>(std::floor(segSCurrPhPos.distanceToPoint(segSBoatPos) * amplitudeCoeff_)));

            auto segFCurrPixPos = globalMesh_.convertPhToPixCoords(segFCurrPhPos);
            auto segSCurrPixPos = globalMesh_.convertPhToPixCoords(segSCurrPhPos);

            // color interpolation between two pixels
            int interpPixX1 = std::min(gMeshWidthPixs - 1,  std::max(0, static_cast<int>(std::round(segFCurrPixPos.x()))));
            int interpPixY1 = std::min(gMeshHeightPixs - 1, std::max(0, static_cast<int>(std::round(segFCurrPixPos.y()))));
            int interpPixX2 = std::min(gMeshWidthPixs - 1,  std::max(0, static_cast<int>(std::round(segSCurrPixPos.x()))));
            int interpPixY2 = std::min(gMeshHeightPixs - 1, std::max(0, static_cast<int>(std::round(segSCurrPixPos.y()))));
            int interpPixDistX = interpPixX2 - interpPixX1;
            int interpPixDistY = interpPixY2 - interpPixY1;
            float interpPixTotDist = std::sqrt(std::pow(interpPixDistX, 2) + std::pow(interpPixDistY, 2));

            //// height matrix
            //int uinterpX1 = std::min(globalWidth  - 1, std::max(0, static_cast<int>(std::round((segFPixPos.x() - globalMeshOrigin.x())))));
            //int uinterpY1 = std::min(globalHeight - 1, std::max(0, static_cast<int>(std::round((segFPixPos.y() - globalMeshOrigin.y())))));
            //int uinterpX2 = std::min(globalWidth  - 1, std::max(0, static_cast<int>(std::round((segSPixPos.x() - globalMeshOrigin.x())))));
            //int uinterpY2 = std::min(globalHeight - 1, std::max(0, static_cast<int>(std::round((segSPixPos.y() - globalMeshOrigin.y())))));
            //int uinterpDistX = uinterpX2 - uinterpX1;
            //int uinterpDistY = uinterpY2 - uinterpY1;

            //// meas line (old)
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
                    float interpProgressByPixel = static_cast<float>(step) / interpPixTotDist;
                    int interpX = interpPixX1 + interpProgressByPixel * interpPixDistX;
                    int interpY = interpPixY1 + interpProgressByPixel * interpPixDistY;
                    auto interpColorIndx = static_cast<int>((1 - interpProgressByPixel) * segFColorIndx + interpProgressByPixel * segSColorIndx);

                    //// height matrix
                    //int uinterpX = uinterpX1 + interpProgress * uinterpDistX;
                    //int uinterpY = uinterpY1 + interpProgress * uinterpDistY;

                    for (int offsetX = -interpLineWidth_; offsetX <= interpLineWidth_; ++offsetX) {
                        for (int offsetY = -interpLineWidth_; offsetY <= interpLineWidth_; ++offsetY) {
                            int bypassInterpX = std::min(gMeshWidthPixs - 1, std::max(0, interpX + offsetX));
                            int bypassInterpY = std::min(gMeshHeightPixs - 1, std::max(0, interpY + offsetY));

                            int meshIndxX = bypassInterpX / globalMesh_.getTileSidePixelSize();
                            int meshIndxY = (globalMesh_.getNumHeightTiles() - 1) - bypassInterpY / globalMesh_.getTileSidePixelSize();
                            int tileIndxX = bypassInterpX % globalMesh_.getTileSidePixelSize();
                            int tileIndxY = bypassInterpY % globalMesh_.getTileSidePixelSize();

                            auto& imageRef = globalMesh_.getTileMatrixRef()[meshIndxY][meshIndxX]->getImageRef();
                            int bytesPerLine = imageRef.bytesPerLine();
                            int bytesInPix = bytesPerLine / imageRef.width();
                            uchar* pixPtr = imageRef.bits() + tileIndxY * bytesPerLine + tileIndxX * bytesInPix;
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
            if (segFPixX1 == segFPixX2 && segFPixY1 == segFPixY2) {
                break;
            }

            // Bresenham
            int segFPixE2 = 2 * segFPixErr;
            if (segFPixE2 > -segFPixDy) {
                segFPixErr -= segFPixDy;
                segFPixX1 += segFPixSx;
            }
            if (segFPixE2 < segFPixDx) {
                segFPixErr += segFPixDx;
                segFPixY1 += segFPixSy;
            }
            int segSPixE2 = 2 * segSPixErr;
            if (segSPixE2 > -segSPixDy) {
                segSPixErr -= segSPixDy;
                segSPixX1 += segSPixSx;
            }
            if (segSPixE2 < segSPixDx) {
                segSPixErr += segSPixDx;
                segSPixY1 += segSPixSy;
            }
        }
    }

    auto renderImpl = RENDER_IMPL(SideScanView);

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

void SideScanView::setView(GraphicsScene3dView *viewPtr)
{
    SceneObject::m_view = viewPtr;
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

void SideScanView::setMeasLineVisible(bool state)
{
    RENDER_IMPL(SideScanView)->measLineVisible_ = state;

    Q_EMIT changed();
}

void SideScanView::setTileGridVisible(bool state)
{
    RENDER_IMPL(SideScanView)->tileGridVisible_ = state;

    Q_EMIT changed();
}

void SideScanView::setTilePixelSize(int size)
{
    tileSidePixelSize_ = size;
}

void SideScanView::setTileHeightMatrixRatio(int ratio)
{
    tileHeightMatrixRatio_ = ratio;
}

void SideScanView::setTileResolution(float resolution)
{
    tileResolution_ = resolution;
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

bool SideScanView::checkLength(float dist) const
{
    if (qFuzzyCompare(1.0f, 1.0f + dist) || (dist < 0.0f)) {
        return false;
    }
    return true;
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

    retVal.width = static_cast<int>(std::ceil(maxX->x() -  minX->x()));
    retVal.height = static_cast<int>(std::ceil(maxY->y() - minY->y()));

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

    int maxX = std::max(srcDst.originX + srcDst.width, src.originX + src.width);
    int maxY = std::max(srcDst.originY + srcDst.height, src.originY + src.height);

    srcDst.originX = std::min(srcDst.originX, src.originX);
    srcDst.originY = std::min(srcDst.originY, src.originY);

    srcDst.width = static_cast<int>(std::ceil(maxX - srcDst.originX));
    srcDst.height = static_cast<int>(std::ceil(maxY - srcDst.originY));
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

/////////////////////////////////////////////////////////////////////////////////////////////////
// SideScanViewRenderImplementation
SideScanView::SideScanViewRenderImplementation::SideScanViewRenderImplementation() :
    tileGridVisible_(false),
    measLineVisible_(true)
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

    // tiles
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

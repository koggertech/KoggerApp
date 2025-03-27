#include "side_scan_view.h"

#include <QtMath>
#include "graphicsscene3dview.h"
#include <QtConcurrent/QtConcurrent>


using namespace sscan;

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
    globalMesh_(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_),
    useLinearFilter_(false),
    trackLastEpoch_(true),
    colorMapTextureId_(0),
    workMode_(Mode::kUndefined),
    manualSettedChannels_(false),
    lAngleOffset_(0.0f),
    rAngleOffset_(0.0f),
    startedInThread_(false)
{
    qRegisterMetaType<Mode>("Mode");
    colorTableTextureTask_ = colorTable_.getRgbaColors();
}

SideScanView::~SideScanView()
{
    for (const auto &itmI : globalMesh_.getTileMatrixRef()) {
        for (const auto& itmJ : itmI) {
            tileTextureTasks_[itmJ->getUuid()] = std::vector<uint8_t>();
        }
    }
}

bool SideScanView::updateChannelsIds()
{
    bool retVal = false;

    segFChannelId_ = -1;
    segSChannelId_ = -1;

    if (datasetPtr_) {
        if (auto chList = datasetPtr_->channelsList(); !chList.empty()) {
            auto it = chList.begin();
            segFChannelId_ = it.key();

            if (++it != chList.end()) {
                segSChannelId_ = it.key();
            }

            retVal = true;
        }
    }

    return retVal;
}

void SideScanView::startUpdateDataInThread(int endIndx, int endOffset)
{
    QThreadPool* threadPool = QThreadPool::globalInstance();

    QtConcurrent::run(threadPool, [this, endIndx, endOffset]() {
        updateData(endIndx, endOffset, true);
    });

    startedInThread_ = true;
    emit sendStartedInThread(startedInThread_);
}

void SideScanView::updateData(int endIndx, int endOffset, bool backgroundThread)
{
    std::unique_ptr<QMutexLocker> locker;
    std::function<void()> cleanFunc;
    if (backgroundThread) {
        cleanFunc = [&, this]() {
            if (backgroundThread) {
                startedInThread_ = false;
                emit sendStartedInThread(startedInThread_);
            }
        };
        locker = std::make_unique<QMutexLocker>(&mutex_);
    }

    if (!datasetPtr_) {
        if (cleanFunc) cleanFunc();
        return;
    }

    if (!manualSettedChannels_ && !updateChannelsIds()) {
        if (cleanFunc) cleanFunc();
        return;
    }

    bool segFIsValid = checkChannel(segFChannelId_);
    bool segSIsValid = checkChannel(segSChannelId_);

    if (!segFIsValid && !segSIsValid) {
        if (cleanFunc) cleanFunc();
        return;
    }

    int epochCount = (endIndx == 0 ? datasetPtr_->size() : endIndx) - endOffset;
    if (epochCount < 4) {
        if (cleanFunc) cleanFunc();
        return;
    }

    // prepare intermediate data (selecting epochs to process)
    MatrixParams actualMatParams(lastMatParams_);
    MatrixParams newMatrixParams;
    QVector<QVector3D> measLinesVertices;
    QVector<int> measLinesEvenIndices;
    QVector<int> measLinesOddIndices;
    QVector<char> isOdds;
    QVector<int> epochIndxs;

    for (int i = lastAcceptedEpoch_; i < epochCount; ++i) {
        bool isAcceptedEpoch = false;

        if (auto epoch = datasetPtr_->fromIndex(i); epoch) {
            auto pos = epoch->getInterpNED();
            auto yaw = epoch->getInterpYaw();
            if (isfinite(pos.n) && isfinite(pos.e) && isfinite(yaw)) {
                bool acceptedEven = false, acceptedOdd = false;
                double azRad = qDegreesToRadians(yaw);

                if (segFIsValid) {
                    if (auto segFCharts = epoch->chart(segFChannelId_); segFCharts) {
                        double leftAzRad = azRad - M_PI_2 + qDegreesToRadians(lAngleOffset_);
                        float lDist = segFCharts->range();

                        measLinesVertices.append(QVector3D(pos.n + lDist * qCos(leftAzRad), pos.e + lDist * qSin(leftAzRad), 0.0f));
                        measLinesVertices.append(QVector3D(pos.n, pos.e, 0.0f));
                        measLinesEvenIndices.append(currIndxSec_++);
                        measLinesEvenIndices.append(currIndxSec_++);
                        isOdds.append('0');
                        epochIndxs.append(i);
                        acceptedEven = true;
                    }
                }

                if (segSIsValid) {
                    if (auto segSCharts = epoch->chart(segSChannelId_); segSCharts) {
                        double rightAzRad = azRad + M_PI_2 - qDegreesToRadians(rAngleOffset_);
                        float rDist = segSCharts ->range();

                        measLinesVertices.append(QVector3D(pos.n, pos.e, 0.0f));
                        measLinesVertices.append(QVector3D(pos.n + rDist * qCos(rightAzRad), pos.e + rDist * qSin(rightAzRad), 0.0f));
                        measLinesOddIndices.append(currIndxSec_++);
                        measLinesOddIndices.append(currIndxSec_++);
                        isOdds.append('1');
                        epochIndxs.append(i);
                        acceptedOdd = true;
                    }
                }

                if (acceptedEven || acceptedOdd) {
                    isAcceptedEpoch = true;
                }
            }
        }

        if (isAcceptedEpoch) {
            lastAcceptedEpoch_ = i;
        }
    }

    lastCalcEpoch_ = epochCount;

    newMatrixParams = getMatrixParams(measLinesVertices);
    if (!newMatrixParams.isValid()) {
        if (cleanFunc) cleanFunc();
        return;
    }

    concatenateMatrixParameters(actualMatParams, newMatrixParams);

    bool meshUpdated = globalMesh_.concatenate(actualMatParams);
    if (meshUpdated) {
        // qDebug() << "actual matrix:";
        // actualMatParams.print(qDebug());
        // qDebug() << "globalmesh:";
        // globalMesh_.printMatrix();
    }

    auto gMeshWidthPixs = globalMesh_.getPixelWidth(); // for bypass
    auto gMeshHeightPixs = globalMesh_.getPixelHeight();

    // processing
    for (int i = 0; i < measLinesVertices.size(); i += 2) { // 2 - step for segment
        if (i + 5 > measLinesVertices.size() - 1) {
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
        Epoch segSEpoch = *segSEpochPtr;
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
        // update compensated TODO: to proc
        if (segFCharts->amplitude.size() != segFCharts->compensated.size()) {
            segFCharts->updateCompesated();
        }
        if (segSCharts->amplitude.size() != segSCharts->compensated.size()) {
            segSCharts->updateCompesated();
        }
        // dist procs checking
        if (!isfinite(segFIsOdd ? segFEpoch.getInterpFirstChannelDist() : segFEpoch.getInterpSecondChannelDist()) ||
            !isfinite(segSIsOdd ? segSEpoch.getInterpFirstChannelDist() : segSEpoch.getInterpSecondChannelDist())) {
            continue;
        }

        // Bresenham
        // first segment
        QVector3D segFPhBegPnt = segFIsOdd ? measLinesVertices[segFBegVertIndx] : measLinesVertices[segFEndVertIndx]; // physics coordinates
        QVector3D segFPhEndPnt = segFIsOdd ? measLinesVertices[segFEndVertIndx] : measLinesVertices[segFBegVertIndx];
        auto segFBegPixPos = globalMesh_.convertPhToPixCoords(segFPhBegPnt);
        auto segFEndPixPos = globalMesh_.convertPhToPixCoords(segFPhEndPnt);
        int segFPixX1 = segFBegPixPos.x();
        int segFPixY1 = segFBegPixPos.y();
        int segFPixX2 = segFEndPixPos.x();
        int segFPixY2 = segFEndPixPos.y();
        float segFPixTotDist = std::sqrt(std::pow(segFPixX2 - segFPixX1, 2) + std::pow(segFPixY2 - segFPixY1, 2));
        int segFPixDx = std::abs(segFPixX2 - segFPixX1);
        int segFPixDy = std::abs(segFPixY2 - segFPixY1);
        int segFPixSx = (segFPixX1 < segFPixX2) ? 1 : -1;
        int segFPixSy = (segFPixY1 < segFPixY2) ? 1 : -1;
        int segFPixErr = segFPixDx - segFPixDy;
        // second segment
        QVector3D segSPhBegPnt = segSIsOdd ? measLinesVertices[segSBegVertIndx] : measLinesVertices[segSEndVertIndx];
        QVector3D segSPhEndPnt = segSIsOdd ? measLinesVertices[segSEndVertIndx] : measLinesVertices[segSBegVertIndx];
        auto segSBegPixPos = globalMesh_.convertPhToPixCoords(segSPhBegPnt);
        auto segSEndPixPos = globalMesh_.convertPhToPixCoords(segSPhEndPnt);
        int segSPixX1 = segSBegPixPos.x();
        int segSPixY1 = segSBegPixPos.y();
        int segSPixX2 = segSEndPixPos.x();
        int segSPixY2 = segSEndPixPos.y();
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

        float segFDistProc = -1.0f * static_cast<float>(segFIsOdd ? segFEpoch.getInterpFirstChannelDist() : segFEpoch.getInterpSecondChannelDist());
        float segSDistProc = -1.0f * static_cast<float>(segSIsOdd ? segSEpoch.getInterpFirstChannelDist() : segSEpoch.getInterpSecondChannelDist());
        float segFPhDistX = segFPhEndPnt.x() - segFPhBegPnt.x();
        float segFPhDistY = segFPhEndPnt.y() - segFPhBegPnt.y();
        float segSPhDistX = segSPhEndPnt.x() - segSPhBegPnt.x();
        float segSPhDistY = segSPhEndPnt.y() - segSPhBegPnt.y();

        auto segFInterpNED = segFEpoch.getInterpNED();
        auto segSInterpNED = segSEpoch.getInterpNED();
        QVector3D segFBoatPos(segFInterpNED.n, segFInterpNED.e, 0.0f);
        QVector3D segSBoatPos(segSInterpNED.n, segSInterpNED.e, 0.0f);

        // follow the first segment
        while (true) {
            // first segment
            float segFPixCurrDist = std::sqrt(std::pow(segFPixX1 - segFPixX2, 2) + std::pow(segFPixY1 - segFPixY2, 2));
            float segFProgByPix = std::min(1.0f, segFPixCurrDist / segFPixTotDist);
            QVector3D segFCurrPhPos(segFPhBegPnt.x() + segFProgByPix * segFPhDistX, segFPhBegPnt.y() + segFProgByPix * segFPhDistY, segFDistProc);
            auto segFColorIndx = getColorIndx(segFCharts, static_cast<int>(std::floor(segFCurrPhPos.distanceToPoint(segFBoatPos) * amplitudeCoeff_)));
            // second segment, calc corresponding progress using smoothed interpolation
            float segSCorrProgByPix = std::min(1.0f, segFPixCurrDist / segFPixTotDist * segSPixTotDist / segFPixTotDist);
            QVector3D segSCurrPhPos(segSPhBegPnt.x() + segSCorrProgByPix * segSPhDistX, segSPhBegPnt.y() + segSCorrProgByPix * segSPhDistY, segSDistProc);
            auto segSColorIndx  = getColorIndx(segSCharts, static_cast<int>(std::floor(segSCurrPhPos.distanceToPoint(segSBoatPos) * amplitudeCoeff_)));

            auto segFCurrPixPos = globalMesh_.convertPhToPixCoords(segFCurrPhPos);
            auto segSCurrPixPos = globalMesh_.convertPhToPixCoords(segSCurrPhPos);

            // color interpolation between two pixels
            int interpPixX1 = segFCurrPixPos.x();
            int interpPixY1 = segFCurrPixPos.y();
            int interpPixX2 = segSCurrPixPos.x();
            int interpPixY2 = segSCurrPixPos.y();
            int interpPixDistX = interpPixX2 - interpPixX1;
            int interpPixDistY = interpPixY2 - interpPixY1;
            float interpPixTotDist = std::sqrt(std::pow(interpPixDistX, 2) + std::pow(interpPixDistY, 2));

            // interpolate
            if (checkLength(interpPixTotDist) && !(segSColorIndx == 0 && segSColorIndx == 0)) {
                for (int step = 0; step <= interpPixTotDist; ++step) {
                    float interpProgressByPixel = static_cast<float>(step) / interpPixTotDist;
                    int interpX = interpPixX1 + interpProgressByPixel * interpPixDistX;
                    int interpY = interpPixY1 + interpProgressByPixel * interpPixDistY;
                    auto interpColorIndx = static_cast<uint8_t>((1 - interpProgressByPixel) * segFColorIndx + interpProgressByPixel * segSColorIndx);

                    for (int offsetX = -interpLineWidth_; offsetX <= interpLineWidth_; ++offsetX) { // bypass
                        for (int offsetY = -interpLineWidth_; offsetY <= interpLineWidth_; ++offsetY) {
                            int bypassInterpX = std::min(gMeshWidthPixs - 1, std::max(0, interpX + offsetX)); // cause bypass
                            int bypassInterpY = std::min(gMeshHeightPixs - 1, std::max(0, interpY + offsetY));

                            int tileSidePixelSize = globalMesh_.getTileSidePixelSize();
                            int meshIndxX = bypassInterpX / tileSidePixelSize;
                            int meshIndxY = (globalMesh_.getNumHeightTiles() - 1) - bypassInterpY / tileSidePixelSize;
                            int tileIndxX = bypassInterpX % tileSidePixelSize;
                            int tileIndxY = bypassInterpY % tileSidePixelSize;

                            auto& tileRef = globalMesh_.getTileMatrixRef()[meshIndxY][meshIndxX];
                            if (!tileRef->getIsInited()) {
                                tileRef->init(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_);
                            }

                            // image
                            auto& imageRef = tileRef->getImageDataRef();
                            int bytesPerLine = std::sqrt(imageRef.size());
                            *(imageRef.data() + tileIndxY * bytesPerLine + tileIndxX) = interpColorIndx;

                            // height matrix
                            int stepSizeHeightMatrix = globalMesh_.getStepSizeHeightMatrix();
                            int numSteps = tileSidePixelSize / stepSizeHeightMatrix + 1;
                            int hVIndx = (tileIndxY / stepSizeHeightMatrix) * numSteps + (tileIndxX / stepSizeHeightMatrix);
                            tileRef->getHeightVerticesRef()[hVIndx][2] = segFCurrPhPos[2];
                            tileRef->getHeightMarkVerticesRef()[hVIndx] = '1';
                            tileRef->setIsUpdate(true);
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

    lastMatParams_ = actualMatParams;

    postUpdate();

    auto renderImpl = RENDER_IMPL(SideScanView);
    renderImpl->measLinesVertices_.append(std::move(measLinesVertices));
    renderImpl->measLinesEvenIndices_.append(std::move(measLinesEvenIndices));
    renderImpl->measLinesOddIndices_.append(std::move(measLinesOddIndices));
    renderImpl->createBounds();

    Q_EMIT changed();
    Q_EMIT boundsChanged();

    if (cleanFunc) cleanFunc();
}

void SideScanView::resetTileSettings(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution)
{
    clear(false);

    tileSidePixelSize_ = tileSidePixelSize;
    tileHeightMatrixRatio_ = tileHeightMatrixRatio;
    tileResolution_ = tileResolution;

    globalMesh_.reinit(tileSidePixelSize, tileHeightMatrixRatio, tileResolution);

    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void SideScanView::clear(bool force)
{
    QMutexLocker locker(&mutex_);

    auto renderImpl = RENDER_IMPL(SideScanView);
    renderImpl->measLinesVertices_.clear();
    renderImpl->measLinesEvenIndices_.clear();
    renderImpl->measLinesOddIndices_.clear();
    renderImpl->createBounds();

    lastCalcEpoch_ = 0;
    lastAcceptedEpoch_ = 0;
    currIndxSec_ = 0;
    lastMatParams_ = MatrixParams();
    if (force) {
        manualSettedChannels_ = false;
    }
    workMode_ = Mode::kUndefined;
    emit sendUpdatedWorkMode(workMode_);

    globalMesh_.clear();

    renderImpl->tiles_.clear();

    for (const auto &itmI : globalMesh_.getTileMatrixRef()) {
        for (const auto& itmJ : itmI) {
            tileTextureTasks_[itmJ->getUuid()] = std::vector<uint8_t>();
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

void SideScanView::setGenerateGridContour(bool state)
{
    globalMesh_.setGenerateGridContour(state);
}

void SideScanView::setColorTableThemeById(int id)
{
    colorTable_.setThemeById(id);
    updateTilesTexture();

    colorTableTextureTask_ = colorTable_.getRgbaColors();

    Q_EMIT changed();
}

void SideScanView::setColorTableLevels(float lowVal, float highVal)
{
    colorTable_.setLevels(lowVal, highVal);
    updateTilesTexture();

    colorTableTextureTask_ = colorTable_.getRgbaColors();

    Q_EMIT changed();
}

void SideScanView::setColorTableLowLevel(float val)
{
    colorTable_.setLowLevel(val);
    updateTilesTexture();

    colorTableTextureTask_ = colorTable_.getRgbaColors();

    Q_EMIT changed();
}

void SideScanView::setColorTableHighLevel(float val)
{
    colorTable_.setHighLevel(val);
    updateTilesTexture();

    colorTableTextureTask_ = colorTable_.getRgbaColors();

    Q_EMIT changed();
}

void SideScanView::setTextureIdByTileId(QUuid tileId, GLuint textureId)
{
    // to tile in storage and render
    // storage
    if (auto* tilePtr = globalMesh_.getTilePtrById(tileId); tilePtr) {
        tilePtr->setTextureId(textureId);
    }

    QMutexLocker locker(&mutex_);

    // render
    auto it = RENDER_IMPL(SideScanView)->tiles_.find(tileId);
    if (it != RENDER_IMPL(SideScanView)->tiles_.end()) {
        it.value().setTextureId(textureId);
        Q_EMIT changed();
    }
}

void SideScanView::setUseLinearFilter(bool state)
{
    useLinearFilter_ = state;

    updateTilesTexture();

    Q_EMIT changed();
}

void SideScanView::setTrackLastEpoch(bool state)
{
    trackLastEpoch_ = state;
}

void SideScanView::setColorTableTextureId(GLuint value)
{
    colorMapTextureId_ = value;

    RENDER_IMPL(SideScanView)->colorTableTextureId_ = colorMapTextureId_;
}

void SideScanView::setWorkMode(Mode mode)
{
    workMode_ = mode;

    emit sendUpdatedWorkMode(workMode_);
}

void SideScanView::setLAngleOffset(float val)
{
    if (val < -90.0f || val > 90.0f) {
        return;
    }

    lAngleOffset_ = val;
}

void SideScanView::setRAngleOffset(float val)
{
    if (val < -90.0f || val > 90.0f) {
        return;
    }

    rAngleOffset_ = val;
}

void SideScanView::setChannels(int firstChId, int secondChId)
{
    segFChannelId_ = firstChId;
    segSChannelId_ = secondChId;
    manualSettedChannels_ = true;
}

GLuint SideScanView::getTextureIdByTileId(QUuid tileId)
{
    QMutexLocker locker(&mutex_);

    // from render
    GLuint retVal = 0;
    auto it = RENDER_IMPL(SideScanView)->tiles_.find(tileId);

    if (it != RENDER_IMPL(SideScanView)->tiles_.end()) {
        retVal =  it.value().getTextureId();
    }

    return retVal;
}

bool SideScanView::getUseLinearFilter() const
{
    return useLinearFilter_;
}

bool SideScanView::getTrackLastEpoch() const
{
    return trackLastEpoch_;
}


GLuint SideScanView::getColorTableTextureId() const
{
    return colorMapTextureId_;
}

QHash<QUuid, std::vector<uint8_t>> SideScanView::getTileTextureTasks()
{
    QWriteLocker locker(&rWLocker_);

    auto retVal = std::move(tileTextureTasks_);

    return retVal;
}

std::vector<uint8_t> SideScanView::getColorTableTextureTask()
{
    QWriteLocker locker(&rWLocker_);

    auto retVal = std::move(colorTableTextureTask_);

    return retVal;
}

SideScanView::Mode SideScanView::getWorkMode() const
{
    return workMode_;
}

bool SideScanView::checkLength(float dist) const
{
    if (qFuzzyIsNull(dist) || (dist < 0.0f)) {
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
    int retVal{ 0 };

    if (charts->compensated.size() > ampIndx) {
        int cVal = charts->compensated[ampIndx] ;
        cVal = std::min(colorTableSize_, cVal);
        retVal = cVal;
    }
    else {
        return 0;
    }

    if (!retVal) {
        return ++retVal;
    }

    return retVal;
}

void SideScanView::postUpdate()
{
    if (!globalMesh_.getIsInited()) {
        return;
    }

    auto updateTextureInView = [this](Tile* tilePtr, bool isNew) -> void {
        if (!isNew) {
            updateUnmarkedHeightVertices(tilePtr);
        }
        tilePtr->updateHeightIndices();
        RENDER_IMPL(SideScanView)->tiles_.insert(tilePtr->getUuid(), *tilePtr); // copy data to render
        tileTextureTasks_[tilePtr->getUuid()] = tilePtr->getImageDataRef();
    };

    int tileMatrixYSize = globalMesh_.getTileMatrixRef().size();
    int tileMatrixXSize = globalMesh_.getTileMatrixRef().at(0).size();

    for (int i = 0; i < tileMatrixYSize; ++i) {
        for (int j = 0; j < tileMatrixXSize; ++j) {

            auto& tileRef = globalMesh_.getTileMatrixRef()[i][j];
            if (tileRef->getIsUpdate()) {
                updateTextureInView(tileRef, false);
                tileRef->setIsUpdate(false);

                // fix height matrixs
                auto& tileVertRef = tileRef->getHeightVerticesRef();
                int numHeightVertBySide = std::sqrt(tileVertRef.size());

                int yIndx = i + 1; // by row
                if (tileMatrixYSize > yIndx) {
                    auto& rowTileRef = globalMesh_.getTileMatrixRef()[yIndx][j];
                    if (!rowTileRef->getIsInited()) {
                        rowTileRef->init(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_);
                    }

                    int topStartIndx = numHeightVertBySide * (numHeightVertBySide - 1);
                    auto& topTileVertRef = rowTileRef->getHeightVerticesRef();
                    auto& topTileMarkVertRef = rowTileRef->getHeightMarkVerticesRef();
                    for (int k = 0; k < numHeightVertBySide; ++k) {
                        int rowIndxTo = topStartIndx + k;
                        int rowIndxFrom = k;
                        if (qFuzzyIsNull(tileVertRef[rowIndxFrom][2])) {
                            continue;
                        }
                        topTileVertRef[rowIndxTo][2] = tileVertRef[rowIndxFrom][2];
                        topTileMarkVertRef[rowIndxTo] = '1';
                    }
                    updateTextureInView(rowTileRef, true);
                }

                int xIndx = j - 1; // by column
                if (xIndx > -1 && tileMatrixXSize > xIndx) {
                    auto& colTileRef = globalMesh_.getTileMatrixRef()[i][xIndx];
                    if (!colTileRef->getIsInited()) {
                        colTileRef->init(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_);
                    }

                    auto& leftTileVertRef = colTileRef->getHeightVerticesRef();
                    auto& leftTileMarkVertRef = colTileRef->getHeightMarkVerticesRef();
                    for (int k = 0; k < numHeightVertBySide; ++k) {
                        int colIndxTo = ((k + 1) * numHeightVertBySide - 1);
                        int colIndxFrom = (k == 0 ? 0 : k * numHeightVertBySide);
                        if (qFuzzyIsNull(tileVertRef[colIndxFrom][2])) {
                            continue;
                        }
                        leftTileVertRef[colIndxTo][2] = tileVertRef[colIndxFrom][2];
                        leftTileMarkVertRef[colIndxTo] = '1';
                    }
                    updateTextureInView(colTileRef, true);
                }
            }
        }
    }
}

void SideScanView::updateTilesTexture()
{
    if (!globalMesh_.getIsInited() || !m_view) {
        return;
    }
    QMutexLocker locker(&mutex_);

    for (auto& itmI : globalMesh_.getTileMatrixRef()) {
        for (auto& itmJ : itmI) {
            tileTextureTasks_[itmJ->getUuid()] = itmJ->getImageDataRef();
        }
    }
}

void SideScanView::updateUnmarkedHeightVertices(Tile* tilePtr) const
{
    if (!tilePtr) {
        return;
    }

    auto& heightVerticesRef = tilePtr->getHeightVerticesRef();
    auto& heightMarkVerticesRef = tilePtr->getHeightMarkVerticesRef();
    int hVSize = heightVerticesRef.size();
    int hMVSize = heightMarkVerticesRef.size();

    if (hVSize != hMVSize) {
        return;
    }

    auto writeHeight = [&](int toIndx, int fromIndx) -> bool {
        if (fromIndx >= 0 && fromIndx < hVSize) {
            if (heightMarkVerticesRef[fromIndx] == '1') {
                heightVerticesRef[toIndx][2] = heightVerticesRef[fromIndx][2];
                return true;
            }
        }
        return false;
    };

    int sideSize = std::sqrt(hVSize);
    for (int i = 0; i < hVSize; ++i) {
        if (heightMarkVerticesRef[i] == '0') {
            if (i % sideSize) {
                if (writeHeight(i, i - 1) ||
                    writeHeight(i, (i - 1) - sideSize) ||
                    writeHeight(i, (i - 1) + sideSize)) {
                    continue;
                }
            }
            if ((i % sideSize + 1) < sideSize) {
                if (writeHeight(i, i + 1) ||
                writeHeight(i, (i + 1) - sideSize)||
                    writeHeight(i, (i + 1) + sideSize)) {
                    continue;
                }
            }
            if (writeHeight(i, i - sideSize) ||
                writeHeight(i, i + sideSize)) {
                continue;
            }
        }
    }
}

bool SideScanView::checkChannel(int val) const
{
    if (val == CHANNEL_NONE || val == CHANNEL_FIRST) {
        return false;
    }
    else {
        return true;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// SideScanViewRenderImplementation
SideScanView::SideScanViewRenderImplementation::SideScanViewRenderImplementation() :
    tileGridVisible_(false),
    measLineVisible_(false),
    colorTableTextureId_(0)
{
#if defined(Q_OS_ANDROID) || defined(LINUX_ES)
    colorTableTextureType_ = GL_TEXTURE_2D;
#else
    colorTableTextureType_ = GL_TEXTURE_1D;
#endif
}

void SideScanView::SideScanViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp,
                                                            const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible) {
        return;
    }

    {
        auto shaderProgram = shaderProgramMap.value("mosaic", nullptr);
        if (!shaderProgram) {
            qWarning() << "Shader program 'mosaic' not found!";
            return;
        }

        // tiles
        for (auto& itm : tiles_) {
            if (!itm.getIsInited()) {
                continue;
            }
            // grid/contour
            if (tileGridVisible_) {
                itm.getGridRenderImplRef().render(ctx, mvp, shaderProgramMap);
                itm.getContourRenderImplRef().render(ctx, mvp, shaderProgramMap);
            }

            shaderProgram->bind();
            shaderProgram->setUniformValue("mvp", mvp);

            int positionLoc = shaderProgram->attributeLocation("position");
            int texCoordLoc = shaderProgram->attributeLocation("texCoord");

            shaderProgram->enableAttributeArray(positionLoc);
            shaderProgram->enableAttributeArray(texCoordLoc);

            shaderProgram->setAttributeArray(positionLoc , itm.getHeightVerticesConstRef().constData());
            shaderProgram->setAttributeArray(texCoordLoc, itm.getTextureVerticesRef().constData());

            {
                QOpenGLFunctions* glFuncs = QOpenGLContext::currentContext()->functions();

                if (itm.getTextureId()) {
                    glFuncs->glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, itm.getTextureId());
                    shaderProgram->setUniformValue("indexedTexture", 0);
                }

                if (colorTableTextureId_) {
                    glFuncs->glActiveTexture(GL_TEXTURE1);
                    glBindTexture(colorTableTextureType_, colorTableTextureId_);
                    shaderProgram->setUniformValue("colorTable", 1);
                }
            }

            ctx->glDrawElements(GL_TRIANGLES, itm.getHeightIndicesRef().size(), GL_UNSIGNED_INT, itm.getHeightIndicesRef().constData());

            shaderProgram->disableAttributeArray(texCoordLoc);
            shaderProgram->disableAttributeArray(positionLoc);

            shaderProgram->release();
        }
    }

    {
        auto shaderProgram = shaderProgramMap.value("static", nullptr);
        if (!shaderProgram) {
            qWarning() << "Shader program 'static' not found!";
            return;
        }

        shaderProgram->bind();
        shaderProgram->setUniformValue("mvp", mvp);

        int posLoc = shaderProgram->attributeLocation("position");
        shaderProgram->enableAttributeArray(posLoc);

        // main line
        if (measLinesVertices_.size() >= 4) {
            shaderProgram->setUniformValue("color", QVector4D(0.031f, 0.69f, 0.98f, 1.0f));
            glLineWidth(3.0f);
            QVector<QVector3D> lastFourVertices = measLinesVertices_.mid(measLinesVertices_.size() - 4, 4);
            shaderProgram->setAttributeArray(posLoc, lastFourVertices.constData());
            ctx->glDrawArrays(GL_LINES, 0, 4);
            glLineWidth(1.0f);
        }

        // measure lines
        if (measLineVisible_) {
            shaderProgram->setAttributeArray(posLoc, measLinesVertices_.constData());
            shaderProgram->setUniformValue("color", QVector4D(0.0f, 1.0f, 0.0f, 1.0f));
            ctx->glDrawElements(GL_LINES, measLinesEvenIndices_.size(), GL_UNSIGNED_INT, measLinesEvenIndices_.constData());
            shaderProgram->setUniformValue("color", QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
            ctx->glDrawElements(GL_LINES, measLinesOddIndices_.size(), GL_UNSIGNED_INT, measLinesOddIndices_.constData());
        }

        shaderProgram->disableAttributeArray(posLoc);
        shaderProgram->release();
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

    m_bounds = Cube(x_min, x_max, y_min, y_max, z_min - 20.f, z_max);
}

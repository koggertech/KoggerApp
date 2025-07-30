#include "mosaic_processor.h"

#include <QtMath>
#include <QDebug>
#include "data_processor.h"
#include "dataset.h"


MosaicProcessor::MosaicProcessor(DataProcessor* parent)
    : dataProcessor_(parent),
    datasetPtr_(nullptr),
    surfaceMeshPtr_(nullptr),
    tileResolution_(defaultTileResolution),
    currIndxSec_(0),
    segFSubChannelId_(0),
    segSSubChannelId_(0),
    tileSidePixelSize_(defaultTileSidePixelSize),
    tileHeightMatrixRatio_(defaultTileHeightMatrixRatio),
    lastCalcEpoch_(0),
    lastAcceptedEpoch_(0),
    lAngleOffset_(0.0f),
    rAngleOffset_(0.0f),
    generateGridContour_(false)
{
}

MosaicProcessor::~MosaicProcessor()
{
}

void MosaicProcessor::clear()
{
    lastMatParams_ = MatrixParams();
    currIndxSec_ = 0;

    segFChannelId_ = ChannelId();
    segFSubChannelId_ = 0;
    segSChannelId_ = ChannelId();
    segSSubChannelId_ = 0;

    lastCalcEpoch_ = 0;
    lastAcceptedEpoch_ = 0;
}

void MosaicProcessor::setDatasetPtr(Dataset *datasetPtr)
{
    datasetPtr_ = datasetPtr;
}

void MosaicProcessor::setSurfaceMeshPtr(SurfaceMesh *surfaceMeshPtr)
{
    surfaceMeshPtr_ = surfaceMeshPtr;
}

void MosaicProcessor::setChannels(const ChannelId& firstChId, uint8_t firstSubChId, const ChannelId& secondChId, uint8_t secondSubChId)
{
    //qDebug() << "MosaicProcessor::setChannels" << firstChId.toShortName() << firstSubChId << secondChId.toShortName() << secondSubChId;

    segFChannelId_    = firstChId;
    segFSubChannelId_ = firstSubChId;
    segSChannelId_    = secondChId;
    segSSubChannelId_ = secondSubChId;
}

void MosaicProcessor::updateDataWrapper(int endIndx, int endOffset)
{
    //qDebug() << "MosaicProcessor::startUpdateData" << endIndx << endOffset;

    dataProcessor_->changeState(DataProcessorType::kMosaic);

    updateData(endIndx, endOffset);

    dataProcessor_->changeState(DataProcessorType::kUndefined);
}

void MosaicProcessor::resetTileSettings(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution)//
{
    clear();

    tileSidePixelSize_ = tileSidePixelSize;
    tileHeightMatrixRatio_ = tileHeightMatrixRatio;
    tileResolution_ = tileResolution;

    surfaceMeshPtr_->reinit(tileSidePixelSize, tileHeightMatrixRatio, tileResolution);//
}

void MosaicProcessor::setGenerateGridContour(bool state)
{
    surfaceMeshPtr_->setGenerateGridContour(state);
}


void MosaicProcessor::setColorTableThemeById(int id)
{
    if (colorTable_.getTheme() == id) {
        return;
    }

    colorTable_.setTheme(id);

    emit dataProcessor_->sendMosaicColorTable(colorTable_.getRgbaColors());
}

void MosaicProcessor::setColorTableLevels(float lowVal, float highVal)
{
    auto levels = colorTable_.getLevels();
    if (qFuzzyCompare(1.0 + levels.first, 1.0 + lowVal) &&
        qFuzzyCompare(1.0 + levels.second, 1.0 + highVal)) {
        return;
    }

    colorTable_.setLevels(lowVal, highVal);

    emit dataProcessor_->sendMosaicColorTable(colorTable_.getRgbaColors());
}

void MosaicProcessor::setColorTableLowLevel(float val)
{
    if (qFuzzyCompare(1.0 + colorTable_.getLowLevel(), 1.0 + val)) {
        return;
    }

    colorTable_.setLowLevel(val);

    emit dataProcessor_->sendMosaicColorTable(colorTable_.getRgbaColors());
}

void MosaicProcessor::setColorTableHighLevel(float val)
{
    if (qFuzzyCompare(1.0 + colorTable_.getHighLevel(), 1.0 + val)) {
        return;
    }

    colorTable_.setHighLevel(val);

    emit dataProcessor_->sendMosaicColorTable(colorTable_.getRgbaColors());
}

void MosaicProcessor::setLAngleOffset(float val)
{
    if (val < -90.0f || val > 90.0f) {
        return;
    }

    lAngleOffset_ = val;
}

void MosaicProcessor::setRAngleOffset(float val)
{
    if (val < -90.0f || val > 90.0f) {
        return;
    }

    rAngleOffset_ = val;
}

void MosaicProcessor::setTileResolution(float tileResolution)
{
    tileResolution_ = tileResolution;
}

void MosaicProcessor::setGenerageGridContour(bool state)
{
    generateGridContour_ = state;
}

void MosaicProcessor::askColorTableForMosaicView()
{
    emit dataProcessor_->sendMosaicColorTable(colorTable_.getRgbaColors());
}

void MosaicProcessor::postUpdate()
{
    //qDebug() << "tileResolution_" << tileResolution_;

    if (!surfaceMeshPtr_->getIsInited()) {
        return;
    }

    auto updateVerticesIndices = [this](SurfaceTile* tilePtr, bool isNew) -> void {
        if (!isNew) {
            updateUnmarkedHeightVertices(tilePtr);
        }
        tilePtr->updateHeightIndices();
    };

    int tileMatrixYSize = surfaceMeshPtr_->getTileMatrixRef().size();
    int tileMatrixXSize = surfaceMeshPtr_->getTileMatrixRef().at(0).size();

    for (int i = 0; i < tileMatrixYSize; ++i) {
        for (int j = 0; j < tileMatrixXSize; ++j) {

            auto& tileRef = surfaceMeshPtr_->getTileMatrixRef()[i][j];
            if (!tileRef->getIsPostUpdate()) {
                continue;
            }

            updateVerticesIndices (tileRef, false);
            tileRef->setIsPostUpdate(false);

            // fix height matrixs
            auto& tileVertRef = tileRef->getHeightVerticesRef();
            int numHeightVertBySide = std::sqrt(tileVertRef.size());

            int yIndx = i + 1; // by row
            if (tileMatrixYSize > yIndx) {
                auto& rowTileRef = surfaceMeshPtr_->getTileMatrixRef()[yIndx][j];
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
                    topTileMarkVertRef[rowIndxTo] = HeightType::kMosaic;
                }
                updateVerticesIndices (rowTileRef, true);
            }

            int xIndx = j - 1; // by column
            if (xIndx > -1 && tileMatrixXSize > xIndx) {
                auto& colTileRef = surfaceMeshPtr_->getTileMatrixRef()[i][xIndx];
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
                    leftTileMarkVertRef[colIndxTo] = HeightType::kMosaic;
                }
                updateVerticesIndices (colTileRef, true);
            }

        }
    }

    // to MosaicView
    const auto& tilesRef = surfaceMeshPtr_->getTilesCRef();
    QHash<QUuid, SurfaceTile> res;
    res.reserve(tilesRef.size());
    for (auto it = tilesRef.begin(); it != tilesRef.cend(); ++it) {
        res.insert((*it)->getUuid(), (*(*it)));
    }
    emit dataProcessor_->sendMosaicTiles(res);
}

void MosaicProcessor::updateUnmarkedHeightVertices(SurfaceTile* tilePtr) const
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
            if (heightMarkVerticesRef[fromIndx] == HeightType::kMosaic) {
                heightVerticesRef[toIndx][2] = heightVerticesRef[fromIndx][2];
                return true;
            }
        }
        return false;
    };

    int sideSize = std::sqrt(hVSize);
    for (int i = 0; i < hVSize; ++i) {
        if (heightMarkVerticesRef[i] == HeightType::kUndefined) {
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

void MosaicProcessor::updateData(int endIndx, int endOffset)
{
    if (!datasetPtr_) {
        return;
    }


    bool segFIsValid = segFChannelId_.isValid();
    bool segSIsValid = segSChannelId_.isValid();

    if (!segFIsValid && !segSIsValid) {
        return;
    }

    int epochCount = (endIndx == 0 ? datasetPtr_->size() : endIndx) - endOffset;
    if (epochCount < 4) {
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
            auto pos = epoch->getPositionGNSS().ned;
            auto yaw = epoch->yaw();
            if (isfinite(pos.n) && isfinite(pos.e) && isfinite(yaw)) {
                bool acceptedEven = false, acceptedOdd = false;
                double azRad = qDegreesToRadians(yaw);

                if (segFIsValid) {
                    if (auto segFCharts = epoch->chart(segFChannelId_, segFSubChannelId_); segFCharts) {
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
                    if (auto segSCharts = epoch->chart(segSChannelId_, segSSubChannelId_); segSCharts) {
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
        return;
    }

    concatenateMatrixParameters(actualMatParams, newMatrixParams);

    bool meshUpdated = surfaceMeshPtr_->concatenate(actualMatParams);
    if (meshUpdated) {
        // qDebug() << "actual matrix:";
        // actualMatParams.print(qDebug());
        // qDebug() << "globalmesh:";
        // globalMeshPtr_->printMatrix();
    }

    auto gMeshWidthPixs = surfaceMeshPtr_->getPixelWidth(); // for bypass
    auto gMeshHeightPixs = surfaceMeshPtr_->getPixelHeight();


    constexpr int sampleLimiter = 2;
    auto sampleIndex = [](Epoch::Echogram* echogramPtr, float dist) -> std::optional<int> {
        if (!echogramPtr) {
            return std::nullopt;
        }
        const float resolution = echogramPtr->resolution;
        if (resolution < 0.0f || qFuzzyIsNull(resolution)) {
            return std::nullopt;
        }
        float resDist = dist / resolution;
        if (resDist < 0.f) {
            return std::nullopt;
        }
        int indx = static_cast<int>(std::round(resDist));
        if (indx >= static_cast<int>(echogramPtr->amplitude.size()) - sampleLimiter) {
            return std::nullopt;
        }
        return indx;
    };

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
        auto segFCharts = segFEpoch.chart(segFIsOdd ? segSChannelId_ : segFChannelId_, segFIsOdd ? segSSubChannelId_ : segFSubChannelId_);
        auto segSCharts = segSEpoch.chart(segSIsOdd ? segSChannelId_ : segFChannelId_, segSIsOdd ? segSSubChannelId_ : segFSubChannelId_);
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
        if (!isfinite(segFIsOdd ? segFEpoch.chart(segFChannelId_, segFSubChannelId_)->bottomProcessing.getDistance() : segFEpoch.chart(segSChannelId_, segSSubChannelId_)->bottomProcessing.getDistance()) ||
            !isfinite(segSIsOdd ? segSEpoch.chart(segFChannelId_, segFSubChannelId_)->bottomProcessing.getDistance() : segSEpoch.chart(segSChannelId_, segSSubChannelId_)->bottomProcessing.getDistance())) {
            continue;
        }

        // Bresenham
        // first segment
        QVector3D segFPhBegPnt = segFIsOdd ? measLinesVertices[segFBegVertIndx] : measLinesVertices[segFEndVertIndx]; // physics coordinates
        QVector3D segFPhEndPnt = segFIsOdd ? measLinesVertices[segFEndVertIndx] : measLinesVertices[segFBegVertIndx];
        auto segFBegPixPos = surfaceMeshPtr_->convertPhToPixCoords(segFPhBegPnt);
        auto segFEndPixPos = surfaceMeshPtr_->convertPhToPixCoords(segFPhEndPnt);
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
        auto segSBegPixPos = surfaceMeshPtr_->convertPhToPixCoords(segSPhBegPnt);
        auto segSEndPixPos = surfaceMeshPtr_->convertPhToPixCoords(segSPhEndPnt);
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

        float segFDistProc = -1.0f * static_cast<float>(segFIsOdd ? segFEpoch.chart(segFChannelId_, segFSubChannelId_)->bottomProcessing.getDistance() : segFEpoch.chart(segSChannelId_, segSSubChannelId_)->bottomProcessing.getDistance());
        float segSDistProc = -1.0f * static_cast<float>(segSIsOdd ? segSEpoch.chart(segFChannelId_, segFSubChannelId_)->bottomProcessing.getDistance() : segSEpoch.chart(segSChannelId_, segSSubChannelId_)->bottomProcessing.getDistance());
        float segFPhDistX = segFPhEndPnt.x() - segFPhBegPnt.x();
        float segFPhDistY = segFPhEndPnt.y() - segFPhBegPnt.y();
        float segSPhDistX = segSPhEndPnt.x() - segSPhBegPnt.x();
        float segSPhDistY = segSPhEndPnt.y() - segSPhBegPnt.y();

        auto segFInterpNED = segFEpoch.getPositionGNSS().ned;
        auto segSInterpNED = segSEpoch.getPositionGNSS().ned;
        QVector3D segFBoatPos(segFInterpNED.n, segFInterpNED.e, 0.0f);
        QVector3D segSBoatPos(segSInterpNED.n, segSInterpNED.e, 0.0f);

        // follow the first segment
        while (true) {
            // first segment
            float segFPixCurrDist = std::sqrt(std::pow(segFPixX1 - segFPixX2, 2) + std::pow(segFPixY1 - segFPixY2, 2));
            float segFProgByPix = std::min(1.0f, segFPixCurrDist / segFPixTotDist);
            QVector3D segFCurrPhPos(segFPhBegPnt.x() + segFProgByPix * segFPhDistX, segFPhBegPnt.y() + segFProgByPix * segFPhDistY, segFDistProc);
            // second segment, calc corresponding progress using smoothed interpolation
            float segSCorrProgByPix = std::min(1.0f, segFPixCurrDist / segFPixTotDist * segSPixTotDist / segFPixTotDist);
            QVector3D segSCurrPhPos(segSPhBegPnt.x() + segSCorrProgByPix * segSPhDistX, segSPhBegPnt.y() + segSCorrProgByPix * segSPhDistY, segSDistProc);

            auto indxF = sampleIndex(segFCharts, segFCurrPhPos.distanceToPoint(segFBoatPos));
            auto indxS = sampleIndex(segSCharts, segSCurrPhPos.distanceToPoint(segSBoatPos));
            int segFColorIndx = 0;
            int segSColorIndx = 0;

            bool bothValid = indxF && indxS;
            if (bothValid) {
                segFColorIndx = getColorIndx(segFCharts, *indxF);
                segSColorIndx = getColorIndx(segSCharts, *indxS);
                if (segFColorIndx == 0 && segSColorIndx == 0) {
                    bothValid = false;
                }
            }

            auto segFCurrPixPos = surfaceMeshPtr_->convertPhToPixCoords(segFCurrPhPos);
            auto segSCurrPixPos = surfaceMeshPtr_->convertPhToPixCoords(segSCurrPhPos);

            // color interpolation between two pixels
            int interpPixX1 = segFCurrPixPos.x();
            int interpPixY1 = segFCurrPixPos.y();
            int interpPixX2 = segSCurrPixPos.x();
            int interpPixY2 = segSCurrPixPos.y();
            int interpPixDistX = interpPixX2 - interpPixX1;
            int interpPixDistY = interpPixY2 - interpPixY1;
            float interpPixTotDist = std::sqrt(std::pow(interpPixDistX, 2) + std::pow(interpPixDistY, 2));

            // interpolate
            if (bothValid && checkLength(interpPixTotDist)) {
                for (int step = 0; step <= interpPixTotDist; ++step) {
                    float interpProgressByPixel = static_cast<float>(step) / interpPixTotDist;
                    int interpX = interpPixX1 + interpProgressByPixel * interpPixDistX;
                    int interpY = interpPixY1 + interpProgressByPixel * interpPixDistY;
                    auto interpColorIndx = static_cast<uint8_t>((1 - interpProgressByPixel) * segFColorIndx + interpProgressByPixel * segSColorIndx);

                    for (int offsetX = -interpLineWidth_; offsetX <= interpLineWidth_; ++offsetX) { // bypass
                        for (int offsetY = -interpLineWidth_; offsetY <= interpLineWidth_; ++offsetY) {
                            int bypassInterpX = std::min(gMeshWidthPixs - 1, std::max(0, interpX + offsetX)); // cause bypass
                            int bypassInterpY = std::min(gMeshHeightPixs - 1, std::max(0, interpY + offsetY));

                            int tileSidePixelSize = surfaceMeshPtr_->getTileSidePixelSize();
                            int meshIndxX = bypassInterpX / tileSidePixelSize;
                            int meshIndxY = (surfaceMeshPtr_->getNumHeightTiles() - 1) - bypassInterpY / tileSidePixelSize;
                            int tileIndxX = bypassInterpX % tileSidePixelSize;
                            int tileIndxY = bypassInterpY % tileSidePixelSize;

                            auto& tileRef = surfaceMeshPtr_->getTileMatrixRef()[meshIndxY][meshIndxX];
                            if (!tileRef->getIsInited()) {
                                tileRef->init(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_);
                            }

                            // image
                            auto& imageRef = tileRef->getImageDataRef();
                            int bytesPerLine = std::sqrt(imageRef.size());
                            *(imageRef.data() + tileIndxY * bytesPerLine + tileIndxX) = interpColorIndx;

                            // height matrix
                            int stepSizeHeightMatrix = surfaceMeshPtr_->getStepSizeHeightMatrix();
                            int numSteps = tileSidePixelSize / stepSizeHeightMatrix + 1;
                            int hVIndx = (tileIndxY / stepSizeHeightMatrix) * numSteps + (tileIndxX / stepSizeHeightMatrix);

                            if (tileRef->getHeightMarkVerticesRef()[hVIndx] != HeightType::kIsobaths) {
                                tileRef->getHeightVerticesRef()[hVIndx][2] = segFCurrPhPos[2];
                                tileRef->getHeightMarkVerticesRef()[hVIndx] = HeightType::kMosaic;
                            }

                            tileRef->setIsPostUpdate(true);
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

    //auto renderImpl = RENDER_IMPL(MosaicProcessor);
    //renderImpl->measLinesVertices_.append(std::move(measLinesVertices));
    //renderImpl->measLinesEvenIndices_.append(std::move(measLinesEvenIndices));
    //renderImpl->measLinesOddIndices_.append(std::move(measLinesOddIndices));
    //renderImpl->createBounds();
}

bool MosaicProcessor::checkLength(float dist) const
{
    if (qFuzzyIsNull(dist) || (dist < 0.0f)) {
        return false;
    }
    return true;
}

int MosaicProcessor::getColorIndx(Epoch::Echogram* charts, int ampIndx) const
{
    int retVal{ 0 };

    if (charts->compensated.size() > ampIndx) {
        int cVal = charts->compensated[ampIndx] ;
        cVal = std::min(colorTableSize_, cVal);
        retVal = cVal;
    }
    else {
        return retVal;
    }

    if (!retVal) {
        return ++retVal;
    }

    return retVal;
}

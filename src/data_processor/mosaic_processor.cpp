// #include "mosaic_processor.h"


// #include <QtMath>
// #include <QtConcurrent/QtConcurrent>


// #include <QDebug>
// #include "data_processor.h"
// #include "dataset.h"


// MosaicProcessor::MosaicProcessor(DataProcessor* parent) :
//     dataProcessor_(parent),
//     datasetPtr_(nullptr),
//     globalMesh_(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_)
// {
//     qRegisterMetaType<Mode>("Mode");

// }

// MosaicProcessor::~MosaicProcessor()
// {
// }

// void MosaicProcessor::clear(bool force)
// {
//     // proc
//     lastCalcEpoch_ = 0;
//     lastAcceptedEpoch_ = 0;
//     currIndxSec_ = 0;
//     lastMatParams_ = MatrixParams();
//     workMode_ = Mode::kUndefined;

//     if (force) {
//         segFChannelId_ = ChannelId();
//         segSChannelId_ = ChannelId();
//         segFSubChannelId_ = 0;
//         segSSubChannelId_ = 0;
//         manualSettedChannels_ = false;
//     }
//     emit sendUpdatedWorkMode(workMode_);

//     globalMesh_.clear();

//     for (const auto &itmI : globalMesh_.getTileMatrixRef()) {
//         for (const auto& itmJ : itmI) {
//             tileTextureTasks_[itmJ->getUuid()] = std::vector<uint8_t>();
//         }
//     }

// }

// void MosaicProcessor::setDatasetPtr(Dataset *datasetPtr)
// {
//     datasetPtr_ = datasetPtr;
// }















// bool MosaicProcessor::updateChannelsIds()
// {
//     bool retVal = false;

//     segFChannelId_ = ChannelId();
//     segFSubChannelId_ = 0;
//     segSChannelId_ = ChannelId();
//     segSSubChannelId_ = 0;

//     if (datasetPtr_) {
//         if (auto chVector = datasetPtr_->channelsList(); !chVector.empty()) {
//             auto it = chVector.begin();

//             //auto& fVal = it. value();

//             segFChannelId_ = it->channelId_;
//             segFSubChannelId_ = it->subChannelId_;

//             if (++it != chVector.end()) {
//                 //auto& sVal = it.value();

//                 segSChannelId_ = it->channelId_;
//                 segSSubChannelId_ = it->subChannelId_;
//             }

//             retVal = true;
//         }
//     }

//     return retVal;
// }

// void MosaicProcessor::startUpdateDataInThread(int endIndx, int endOffset)
// {
//     QThreadPool* threadPool = QThreadPool::globalInstance();

//     QtConcurrent::run(threadPool, [this, endIndx, endOffset]() {
//         updateData(endIndx, endOffset, true);
//     });

//     startedInThread_ = true;
//     emit sendStartedInThread(startedInThread_);
// }

// void MosaicProcessor::updateData(int endIndx, int endOffset, bool backgroundThread)
// {
//     Q_UNUSED(endIndx)
//     Q_UNUSED(endOffset)
//     Q_UNUSED(backgroundThread)

//     /*std::unique_ptr<QMutexLocker> locker;
//     std::function<void()> cleanFunc;
//     if (backgroundThread) {
//         cleanFunc = [&, this]() {
//             if (backgroundThread) {
//                 startedInThread_ = false;
//                 emit sendStartedInThread(startedInThread_);
//             }
//         };
//         locker = std::make_unique<QMutexLocker>(&mutex_);
//     }

//     if (!datasetPtr_) {
//         if (cleanFunc) cleanFunc();
//         return;
//     }

//     if (!manualSettedChannels_ && !updateChannelsIds()) {
//         if (cleanFunc) cleanFunc();
//         return;
//     }

//     bool segFIsValid = segFChannelId_.isValid();
//     bool segSIsValid = segSChannelId_.isValid();

//     if (!segFIsValid && !segSIsValid) {
//         if (cleanFunc) cleanFunc();
//         return;
//     }

//     int epochCount = (endIndx == 0 ? datasetPtr_->size() : endIndx) - endOffset;
//     if (epochCount < 4) {
//         if (cleanFunc) cleanFunc();
//         return;
//     }

//     // prepare intermediate data (selecting epochs to process)
//     MatrixParams actualMatParams(lastMatParams_);
//     MatrixParams newMatrixParams;
//     QVector<QVector3D> measLinesVertices;
//     QVector<int> measLinesEvenIndices;
//     QVector<int> measLinesOddIndices;
//     QVector<char> isOdds;
//     QVector<int> epochIndxs;

//     for (int i = lastAcceptedEpoch_; i < epochCount; ++i) {
//         bool isAcceptedEpoch = false;

//         if (auto epoch = datasetPtr_->fromIndex(i); epoch) {
//             auto pos = epoch->getInterpNED();
//             auto yaw = epoch->getInterpYaw();
//             if (isfinite(pos.n) && isfinite(pos.e) && isfinite(yaw)) {
//                 bool acceptedEven = false, acceptedOdd = false;
//                 double azRad = qDegreesToRadians(yaw);

//                 if (segFIsValid) {
//                     if (auto segFCharts = epoch->chart(segFChannelId_, segFSubChannelId_); segFCharts) {
//                         double leftAzRad = azRad - M_PI_2 + qDegreesToRadians(lAngleOffset_);
//                         float lDist = segFCharts->range();

//                         measLinesVertices.append(QVector3D(pos.n + lDist * qCos(leftAzRad), pos.e + lDist * qSin(leftAzRad), 0.0f));
//                         measLinesVertices.append(QVector3D(pos.n, pos.e, 0.0f));
//                         measLinesEvenIndices.append(currIndxSec_++);
//                         measLinesEvenIndices.append(currIndxSec_++);
//                         isOdds.append('0');
//                         epochIndxs.append(i);
//                         acceptedEven = true;
//                     }
//                 }

//                 if (segSIsValid) {
//                     if (auto segSCharts = epoch->chart(segSChannelId_, segSSubChannelId_); segSCharts) {
//                         double rightAzRad = azRad + M_PI_2 - qDegreesToRadians(rAngleOffset_);
//                         float rDist = segSCharts ->range();

//                         measLinesVertices.append(QVector3D(pos.n, pos.e, 0.0f));
//                         measLinesVertices.append(QVector3D(pos.n + rDist * qCos(rightAzRad), pos.e + rDist * qSin(rightAzRad), 0.0f));
//                         measLinesOddIndices.append(currIndxSec_++);
//                         measLinesOddIndices.append(currIndxSec_++);
//                         isOdds.append('1');
//                         epochIndxs.append(i);
//                         acceptedOdd = true;
//                     }
//                 }

//                 if (acceptedEven || acceptedOdd) {
//                     isAcceptedEpoch = true;
//                 }
//             }
//         }

//         if (isAcceptedEpoch) {
//             lastAcceptedEpoch_ = i;
//         }
//     }

//     lastCalcEpoch_ = epochCount;

//     newMatrixParams = getMatrixParams(measLinesVertices);
//     if (!newMatrixParams.isValid()) {
//         if (cleanFunc) cleanFunc();
//         return;
//     }

//     concatenateMatrixParameters(actualMatParams, newMatrixParams);

//     bool meshUpdated = globalMesh_.concatenate(actualMatParams);
//     if (meshUpdated) {
//         // qDebug() << "actual matrix:";
//         // actualMatParams.print(qDebug());
//         // qDebug() << "globalmesh:";
//         // globalMesh_.printMatrix();
//     }

//     auto gMeshWidthPixs = globalMesh_.getPixelWidth(); // for bypass
//     auto gMeshHeightPixs = globalMesh_.getPixelHeight();


//     constexpr int sampleLimiter = 2;
//     auto sampleIndex = [](Epoch::Echogram* echogramPtr, float dist) -> std::optional<int> {
//         if (!echogramPtr) {
//             return std::nullopt;
//         }
//         const float resolution = echogramPtr->resolution;
//         if (resolution < 0.0f || qFuzzyIsNull(resolution)) {
//             return std::nullopt;
//         }
//         float resDist = dist / resolution;
//         if (resDist < 0.f) {
//             return std::nullopt;
//         }
//         int indx = static_cast<int>(std::round(resDist));
//         if (indx >= static_cast<int>(echogramPtr->amplitude.size()) - sampleLimiter) {
//             return std::nullopt;
//         }
//         return indx;
//     };

//     // processing
//     for (int i = 0; i < measLinesVertices.size(); i += 2) { // 2 - step for segment
//         if (i + 5 > measLinesVertices.size() - 1) {
//             break;
//             }

//         int segFBegVertIndx = i;
//         int segFEndVertIndx = i + 1;
//         int segSBegVertIndx = i + 2;
//         int segSEndVertIndx = i + 3;

//         int segFIndx = i / 2;
//         int segSIndx = (i + 2) / 2;
//         // going to next epoch if needed
//         if (epochIndxs[segFIndx] == epochIndxs[segSIndx] ||
//             isOdds[segFIndx] != isOdds[segSIndx]) {
//             ++segSIndx;
//             segSBegVertIndx += 2;
//             segSEndVertIndx += 2;
//         }
//         // compliance check
//         if (epochIndxs[segFIndx] == epochIndxs[segSIndx] ||
//             isOdds[segFIndx] != isOdds[segSIndx]) {
//             continue;
//         }
//         // epochs checking
//         auto segFEpochPtr = datasetPtr_->fromIndex(epochIndxs[segFIndx]);
//         auto segSEpochPtr = datasetPtr_->fromIndex(epochIndxs[segSIndx]);
//         if (!segFEpochPtr || !segSEpochPtr) {
//             continue;
//         }
//         Epoch segFEpoch = *segFEpochPtr; // _plot might be reallocated
//         Epoch segSEpoch = *segSEpochPtr;
//         // isOdd checking
//         bool segFIsOdd = isOdds[segFIndx] == '1';
//         bool segSIsOdd = isOdds[segSIndx] == '1';
//         if (segFIsOdd != segSIsOdd) {
//             continue;
//         }
//         // segments checking
//         auto segFCharts = segFEpoch.chart(segFIsOdd ? segSChannelId_ : segFChannelId_, segFIsOdd ? segSSubChannelId_ : segFSubChannelId_);
//         auto segSCharts = segSEpoch.chart(segSIsOdd ? segSChannelId_ : segFChannelId_, segSIsOdd ? segSSubChannelId_ : segFSubChannelId_);
//         if (!segFCharts || !segSCharts) {
//             continue;
//         }
//         // update compensated TODO: to proc
//         if (segFCharts->amplitude.size() != segFCharts->compensated.size()) {
//             segFCharts->updateCompesated();
//         }
//         if (segSCharts->amplitude.size() != segSCharts->compensated.size()) {
//             segSCharts->updateCompesated();
//         }
//         // dist procs checking
//         if (!isfinite(segFIsOdd ? segFEpoch.getInterpFirstChannelDist() : segFEpoch.getInterpSecondChannelDist()) ||
//             !isfinite(segSIsOdd ? segSEpoch.getInterpFirstChannelDist() : segSEpoch.getInterpSecondChannelDist())) {
//             continue;
//         }

//         // Bresenham
//         // first segment
//         QVector3D segFPhBegPnt = segFIsOdd ? measLinesVertices[segFBegVertIndx] : measLinesVertices[segFEndVertIndx]; // physics coordinates
//         QVector3D segFPhEndPnt = segFIsOdd ? measLinesVertices[segFEndVertIndx] : measLinesVertices[segFBegVertIndx];
//         auto segFBegPixPos = globalMesh_.convertPhToPixCoords(segFPhBegPnt);
//         auto segFEndPixPos = globalMesh_.convertPhToPixCoords(segFPhEndPnt);
//         int segFPixX1 = segFBegPixPos.x();
//         int segFPixY1 = segFBegPixPos.y();
//         int segFPixX2 = segFEndPixPos.x();
//         int segFPixY2 = segFEndPixPos.y();
//         float segFPixTotDist = std::sqrt(std::pow(segFPixX2 - segFPixX1, 2) + std::pow(segFPixY2 - segFPixY1, 2));
//         int segFPixDx = std::abs(segFPixX2 - segFPixX1);
//         int segFPixDy = std::abs(segFPixY2 - segFPixY1);
//         int segFPixSx = (segFPixX1 < segFPixX2) ? 1 : -1;
//         int segFPixSy = (segFPixY1 < segFPixY2) ? 1 : -1;
//         int segFPixErr = segFPixDx - segFPixDy;
//         // second segment
//         QVector3D segSPhBegPnt = segSIsOdd ? measLinesVertices[segSBegVertIndx] : measLinesVertices[segSEndVertIndx];
//         QVector3D segSPhEndPnt = segSIsOdd ? measLinesVertices[segSEndVertIndx] : measLinesVertices[segSBegVertIndx];
//         auto segSBegPixPos = globalMesh_.convertPhToPixCoords(segSPhBegPnt);
//         auto segSEndPixPos = globalMesh_.convertPhToPixCoords(segSPhEndPnt);
//         int segSPixX1 = segSBegPixPos.x();
//         int segSPixY1 = segSBegPixPos.y();
//         int segSPixX2 = segSEndPixPos.x();
//         int segSPixY2 = segSEndPixPos.y();
//         float segSPixTotDist = std::sqrt(std::pow(segSPixX2 - segSPixX1, 2) + std::pow(segSPixY2 - segSPixY1, 2));
//         int segSPixDx = std::abs(segSPixX2 - segSPixX1);
//         int segSPixDy = std::abs(segSPixY2 - segSPixY1);
//         int segSPixSx = (segSPixX1 < segSPixX2) ? 1 : -1;
//         int segSPixSy = (segSPixY1 < segSPixY2) ? 1 : -1;
//         int segSPixErr = segSPixDx - segSPixDy;

//         // pixel length checking
//         if (!checkLength(segFPixTotDist) ||
//             !checkLength(segSPixTotDist)) {
//             continue;
//         }

//         float segFDistProc = -1.0f * static_cast<float>(segFIsOdd ? segFEpoch.getInterpFirstChannelDist() : segFEpoch.getInterpSecondChannelDist());
//         float segSDistProc = -1.0f * static_cast<float>(segSIsOdd ? segSEpoch.getInterpFirstChannelDist() : segSEpoch.getInterpSecondChannelDist());
//         float segFPhDistX = segFPhEndPnt.x() - segFPhBegPnt.x();
//         float segFPhDistY = segFPhEndPnt.y() - segFPhBegPnt.y();
//         float segSPhDistX = segSPhEndPnt.x() - segSPhBegPnt.x();
//         float segSPhDistY = segSPhEndPnt.y() - segSPhBegPnt.y();

//         auto segFInterpNED = segFEpoch.getInterpNED();
//         auto segSInterpNED = segSEpoch.getInterpNED();
//         QVector3D segFBoatPos(segFInterpNED.n, segFInterpNED.e, 0.0f);
//         QVector3D segSBoatPos(segSInterpNED.n, segSInterpNED.e, 0.0f);

//         // follow the first segment
//         while (true) {
//             // first segment
//             float segFPixCurrDist = std::sqrt(std::pow(segFPixX1 - segFPixX2, 2) + std::pow(segFPixY1 - segFPixY2, 2));
//             float segFProgByPix = std::min(1.0f, segFPixCurrDist / segFPixTotDist);
//             QVector3D segFCurrPhPos(segFPhBegPnt.x() + segFProgByPix * segFPhDistX, segFPhBegPnt.y() + segFProgByPix * segFPhDistY, segFDistProc);
//             // second segment, calc corresponding progress using smoothed interpolation
//             float segSCorrProgByPix = std::min(1.0f, segFPixCurrDist / segFPixTotDist * segSPixTotDist / segFPixTotDist);
//             QVector3D segSCurrPhPos(segSPhBegPnt.x() + segSCorrProgByPix * segSPhDistX, segSPhBegPnt.y() + segSCorrProgByPix * segSPhDistY, segSDistProc);

//             auto indxF = sampleIndex(segFCharts, segFCurrPhPos.distanceToPoint(segFBoatPos));
//             auto indxS = sampleIndex(segSCharts, segSCurrPhPos.distanceToPoint(segSBoatPos));
//             int segFColorIndx = 0;
//             int segSColorIndx = 0;

//             bool bothValid = indxF && indxS;
//             if (bothValid) {
//                 segFColorIndx = getColorIndx(segFCharts, *indxF);
//                 segSColorIndx = getColorIndx(segSCharts, *indxS);
//                 if (segFColorIndx == 0 && segSColorIndx == 0) {
//                     bothValid = false;
//                 }
//             }

//             auto segFCurrPixPos = globalMesh_.convertPhToPixCoords(segFCurrPhPos);
//             auto segSCurrPixPos = globalMesh_.convertPhToPixCoords(segSCurrPhPos);

//             // color interpolation between two pixels
//             int interpPixX1 = segFCurrPixPos.x();
//             int interpPixY1 = segFCurrPixPos.y();
//             int interpPixX2 = segSCurrPixPos.x();
//             int interpPixY2 = segSCurrPixPos.y();
//             int interpPixDistX = interpPixX2 - interpPixX1;
//             int interpPixDistY = interpPixY2 - interpPixY1;
//             float interpPixTotDist = std::sqrt(std::pow(interpPixDistX, 2) + std::pow(interpPixDistY, 2));

//             // interpolate
//             if (bothValid && checkLength(interpPixTotDist)) {
//                 for (int step = 0; step <= interpPixTotDist; ++step) {
//                     float interpProgressByPixel = static_cast<float>(step) / interpPixTotDist;
//                     int interpX = interpPixX1 + interpProgressByPixel * interpPixDistX;
//                     int interpY = interpPixY1 + interpProgressByPixel * interpPixDistY;
//                     auto interpColorIndx = static_cast<uint8_t>((1 - interpProgressByPixel) * segFColorIndx + interpProgressByPixel * segSColorIndx);

//                     for (int offsetX = -interpLineWidth_; offsetX <= interpLineWidth_; ++offsetX) { // bypass
//                         for (int offsetY = -interpLineWidth_; offsetY <= interpLineWidth_; ++offsetY) {
//                             int bypassInterpX = std::min(gMeshWidthPixs - 1, std::max(0, interpX + offsetX)); // cause bypass
//                             int bypassInterpY = std::min(gMeshHeightPixs - 1, std::max(0, interpY + offsetY));

//                             int tileSidePixelSize = globalMesh_.getTileSidePixelSize();
//                             int meshIndxX = bypassInterpX / tileSidePixelSize;
//                             int meshIndxY = (globalMesh_.getNumHeightTiles() - 1) - bypassInterpY / tileSidePixelSize;
//                             int tileIndxX = bypassInterpX % tileSidePixelSize;
//                             int tileIndxY = bypassInterpY % tileSidePixelSize;

//                             auto& tileRef = globalMesh_.getTileMatrixRef()[meshIndxY][meshIndxX];
//                             if (!tileRef->getIsInited()) {
//                                 tileRef->init(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_);
//                             }

//                             // image
//                             auto& imageRef = tileRef->getImageDataRef();
//                             int bytesPerLine = std::sqrt(imageRef.size());
//                             *(imageRef.data() + tileIndxY * bytesPerLine + tileIndxX) = interpColorIndx;

//                             // height matrix
//                             int stepSizeHeightMatrix = globalMesh_.getStepSizeHeightMatrix();
//                             int numSteps = tileSidePixelSize / stepSizeHeightMatrix + 1;
//                             int hVIndx = (tileIndxY / stepSizeHeightMatrix) * numSteps + (tileIndxX / stepSizeHeightMatrix);
//                             tileRef->getHeightVerticesRef()[hVIndx][2] = segFCurrPhPos[2];
//                             tileRef->getHeightMarkVerticesRef()[hVIndx] = '1';
//                             tileRef->setIsUpdate(true);
//                         }
//                     }
//                 }
//             }

//             // break at the end of the first segment
//             if (segFPixX1 == segFPixX2 && segFPixY1 == segFPixY2) {
//                 break;
//             }

//             // Bresenham
//             int segFPixE2 = 2 * segFPixErr;
//             if (segFPixE2 > -segFPixDy) {
//                 segFPixErr -= segFPixDy;
//                 segFPixX1 += segFPixSx;
//             }
//             if (segFPixE2 < segFPixDx) {
//                 segFPixErr += segFPixDx;
//                 segFPixY1 += segFPixSy;
//             }
//             int segSPixE2 = 2 * segSPixErr;
//             if (segSPixE2 > -segSPixDy) {
//                 segSPixErr -= segSPixDy;
//                 segSPixX1 += segSPixSx;
//             }
//             if (segSPixE2 < segSPixDx) {
//                 segSPixErr += segSPixDx;
//                 segSPixY1 += segSPixSy;
//             }
//         }
//     }

//     lastMatParams_ = actualMatParams;

//     postUpdate();

//     auto renderImpl = RENDER_IMPL(MosaicProcessor);
//     renderImpl->measLinesVertices_.append(std::move(measLinesVertices));
//     renderImpl->measLinesEvenIndices_.append(std::move(measLinesEvenIndices));
//     renderImpl->measLinesOddIndices_.append(std::move(measLinesOddIndices));
//     renderImpl->createBounds();

//     Q_EMIT changed();
//     Q_EMIT boundsChanged();

//     if (cleanFunc) cleanFunc();*/
// }

// void MosaicProcessor::resetTileSettings(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution)
// {
//     clear(false);

//     tileSidePixelSize_ = tileSidePixelSize;
//     tileHeightMatrixRatio_ = tileHeightMatrixRatio;
//     tileResolution_ = tileResolution;

//     globalMesh_.reinit(tileSidePixelSize, tileHeightMatrixRatio, tileResolution);

//     //Q_EMIT changed();
//     //Q_EMIT boundsChanged();
// }




// void MosaicProcessor::setGenerateGridContour(bool state)
// {
//     globalMesh_.setGenerateGridContour(state);
// }

// void MosaicProcessor::setColorTableThemeById(int id)
// {
//     colorTable_.setTheme(id);
//     updateTilesTexture();

//     colorTableTextureTask_ = colorTable_.getRgbaColors();

//     Q_EMIT changed();
// }

// void MosaicProcessor::setColorTableLevels(float lowVal, float highVal)
// {
//     colorTable_.setLevels(lowVal, highVal);
//     updateTilesTexture();

//     colorTableTextureTask_ = colorTable_.getRgbaColors();

//     Q_EMIT changed();
// }

// void MosaicProcessor::setColorTableLowLevel(float val)
// {
//     colorTable_.setLowLevel(val);
//     updateTilesTexture();

//     colorTableTextureTask_ = colorTable_.getRgbaColors();

//     Q_EMIT changed();
// }

// void MosaicProcessor::setColorTableHighLevel(float val)
// {
//     colorTable_.setHighLevel(val);
//     updateTilesTexture();

//     colorTableTextureTask_ = colorTable_.getRgbaColors();

//     Q_EMIT changed();
// }




// void MosaicProcessor::setWorkMode(Mode mode)
// {
//     workMode_ = mode;

//     emit sendUpdatedWorkMode(workMode_);
// }

// void MosaicProcessor::setLAngleOffset(float val)
// {
//     if (val < -90.0f || val > 90.0f) {
//         return;
//     }

//     lAngleOffset_ = val;
// }

// void MosaicProcessor::setRAngleOffset(float val)
// {
//     if (val < -90.0f || val > 90.0f) {
//         return;
//     }

//     rAngleOffset_ = val;
// }

// void MosaicProcessor::setChannels(const ChannelId& firstChId, uint8_t firstSubChId, const ChannelId& secondChId, uint8_t secondSubChId)
// {
//     segFChannelId_ = firstChId;
//     segFSubChannelId_ = firstSubChId;
//     segSChannelId_ = secondChId;
//     segSSubChannelId_ = secondSubChId;
//     manualSettedChannels_ = true;
// }



// void MosaicProcessor::postUpdate()
// {
//     //if (!globalMesh_.getIsInited()) {
//     //    return;
//     //}

//     auto updateTextureInView = [this](Tile* tilePtr, bool isNew) -> void {
//         if (!isNew) {
//             updateUnmarkedHeightVertices(tilePtr);
//         }
//         tilePtr->updateHeightIndices();
//         if (auto* r = RENDER_IMPL(MosaicProcessor); r) {
//             r->tiles_.insert(tilePtr->getUuid(), *tilePtr); // copy data to render
//             tileTextureTasks_[tilePtr->getUuid()] = tilePtr->getImageDataCRef();
//         }
//     };

//     int tileMatrixYSize = globalMesh_.getTileMatrixRef().size();
//     int tileMatrixXSize = globalMesh_.getTileMatrixRef().at(0).size();

//     for (int i = 0; i < tileMatrixYSize; ++i) {
//         for (int j = 0; j < tileMatrixXSize; ++j) {

//             auto& tileRef = globalMesh_.getTileMatrixRef()[i][j];
//             if (!tileRef->getIsUpdate()) {
//                 continue;
//             }


//             updateTextureInView(tileRef, false);
//             tileRef->setIsUpdate(false);

//             // fix height matrixs
//             auto& tileVertRef = tileRef->getHeightVerticesRef();
//             int numHeightVertBySide = std::sqrt(tileVertRef.size());

//             int yIndx = i + 1; // by row
//             if (tileMatrixYSize > yIndx) {
//                 auto& rowTileRef = globalMesh_.getTileMatrixRef()[yIndx][j];
//                 if (!rowTileRef->getIsInited()) {
//                     rowTileRef->init(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_);
//                 }

//                 int topStartIndx = numHeightVertBySide * (numHeightVertBySide - 1);
//                 auto& topTileVertRef = rowTileRef->getHeightVerticesRef();
//                 auto& topTileMarkVertRef = rowTileRef->getHeightMarkVerticesRef();
//                 for (int k = 0; k < numHeightVertBySide; ++k) {
//                     int rowIndxTo = topStartIndx + k;
//                     int rowIndxFrom = k;
//                     if (qFuzzyIsNull(tileVertRef[rowIndxFrom][2])) {
//                         continue;
//                     }
//                     topTileVertRef[rowIndxTo][2] = tileVertRef[rowIndxFrom][2];
//                     topTileMarkVertRef[rowIndxTo] = '1';
//                 }
//                 updateTextureInView(rowTileRef, true);
//             }

//             int xIndx = j - 1; // by column
//             if (xIndx > -1 && tileMatrixXSize > xIndx) {
//                 auto& colTileRef = globalMesh_.getTileMatrixRef()[i][xIndx];
//                 if (!colTileRef->getIsInited()) {
//                     colTileRef->init(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_);
//                 }

//                 auto& leftTileVertRef = colTileRef->getHeightVerticesRef();
//                 auto& leftTileMarkVertRef = colTileRef->getHeightMarkVerticesRef();
//                 for (int k = 0; k < numHeightVertBySide; ++k) {
//                     int colIndxTo = ((k + 1) * numHeightVertBySide - 1);
//                     int colIndxFrom = (k == 0 ? 0 : k * numHeightVertBySide);
//                     if (qFuzzyIsNull(tileVertRef[colIndxFrom][2])) {
//                         continue;
//                     }
//                     leftTileVertRef[colIndxTo][2] = tileVertRef[colIndxFrom][2];
//                     leftTileMarkVertRef[colIndxTo] = '1';
//                 }
//                 updateTextureInView(colTileRef, true);
//             }

//         }
//     }
// }



// void MosaicProcessor::updateUnmarkedHeightVertices(Tile* tilePtr) const
// {
//     if (!tilePtr) {
//         return;
//     }

//     auto& heightVerticesRef = tilePtr->getHeightVerticesRef();
//     auto& heightMarkVerticesRef = tilePtr->getHeightMarkVerticesRef();
//     int hVSize = heightVerticesRef.size();
//     int hMVSize = heightMarkVerticesRef.size();

//     if (hVSize != hMVSize) {
//         return;
//     }

//     auto writeHeight = [&](int toIndx, int fromIndx) -> bool {
//         if (fromIndx >= 0 && fromIndx < hVSize) {
//             if (heightMarkVerticesRef[fromIndx] == '1') {
//                 heightVerticesRef[toIndx][2] = heightVerticesRef[fromIndx][2];
//                 return true;
//             }
//         }
//         return false;
//     };

//     int sideSize = std::sqrt(hVSize);
//     for (int i = 0; i < hVSize; ++i) {
//         if (heightMarkVerticesRef[i] == '0') {
//             if (i % sideSize) {
//                 if (writeHeight(i, i - 1) ||
//                     writeHeight(i, (i - 1) - sideSize) ||
//                     writeHeight(i, (i - 1) + sideSize)) {
//                     continue;
//                 }
//             }
//             if ((i % sideSize + 1) < sideSize) {
//                 if (writeHeight(i, i + 1) ||
//                     writeHeight(i, (i + 1) - sideSize)||
//                     writeHeight(i, (i + 1) + sideSize)) {
//                     continue;
//                 }
//             }
//             if (writeHeight(i, i - sideSize) ||
//                 writeHeight(i, i + sideSize)) {
//                 continue;
//             }
//         }
//     }
// }



// void MosaicProcessor::setColorTableThemeById(int id)
// {
//     if (colorTable_.getTheme() == id) {
//         return;
//     }

//     colorTable_.setTheme(id);
//     colorTableTextureTask_ = colorTable_.getRgbaColors();
//     updateTexturesForTiles();

//     Q_EMIT changed();
// }

// void MosaicProcessor::setColorTableLevels(float lowVal, float highVal)
// {
//     auto levels = colorTable_.getLevels();
//     if (qFuzzyCompare(1.0 + levels.first, 1.0 + lowVal) &&
//         qFuzzyCompare(1.0 + levels.second, 1.0 + highVal)) {
//         return;
//     }

//     colorTable_.setLevels(lowVal, highVal);
//     colorTableTextureTask_ = colorTable_.getRgbaColors();
//     updateTexturesForTiles();

//     Q_EMIT changed();
// }

// void MosaicProcessor::setColorTableLowLevel(float val)
// {
//     if (qFuzzyCompare(1.0 + colorTable_.getLowLevel(), 1.0 + val)) {
//         return;
//     }

//     colorTable_.setLowLevel(val);
//     colorTableTextureTask_ = colorTable_.getRgbaColors();
//     updateTexturesForTiles();

//     Q_EMIT changed();
// }

// void MosaicProcessor::setColorTableHighLevel(float val)
// {
//     if (qFuzzyCompare(1.0 + colorTable_.getHighLevel(), 1.0 + val)) {
//         return;
//     }

//     colorTable_.setHighLevel(val);
//     colorTableTextureTask_ = colorTable_.getRgbaColors();
//     updateTexturesForTiles();

//     Q_EMIT changed();
// }

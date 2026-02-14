#include "qPlot2D.h"

#include <time.h>
#include <QMutex>
#include <QPixmap>
#include <QPainter>
#include <QSGSimpleTextureNode>
#include <QQuickWindow>
#include <cmath>
#include <limits>
#include "epoch_event.h"


qPlot2D::qPlot2D(QQuickItem* parent)
    : QQuickPaintedItem(parent)
    , m_updateTimer(new QTimer(this))
{
    qRegisterMetaType<ChannelId>("ChannelId");
    qRegisterMetaType<DatasetChannel>("DatasetChannel");

//    setRenderTarget(QQuickPaintedItem::FramebufferObject);
//    connect(m_updateTimer, &QTimer::timeout, this, [&] { update(); });
    m_updateTimer->start(30);
    setFlag(ItemHasContents);
    setAcceptedMouseButtons(Qt::AllButtons);
//    setFillColor(QColor(255, 255, 255));

    _isHorizontal = false;
}

void qPlot2D::paint(QPainter *painter)
{
    if (!Plot2D::plotEnabled()) {
        return;
    }

    clock_t start = clock();

    if (m_plot != nullptr && painter != nullptr) {
        const int w = static_cast<int>(width());
        const int h = static_cast<int>(height());
        if (w <= 0 || h <= 0) {
            return;
        }

        if (zoomPreviewMode_ && datasetPtr_ && datasetPtr_->size() > 0 && zoomPreviewEpochIndx_ >= 0) {
            const float halfWidth = static_cast<float>(qRound(w * 0.5f));
            const float timelinePos = static_cast<float>(zoomPreviewEpochIndx_ + halfWidth) / static_cast<float>(datasetPtr_->size());
            cursor_.position = qBound(0.0f, timelinePos, 1.0f);
            cursor_.selectEpochIndx = zoomPreviewEpochIndx_;
        }

        Plot2D::getImage(w, h, painter, _isHorizontal);

        if (zoomPreviewMode_) {
            int centerX = qBound(0, canvas().width() / 2, canvas().width() - 1);
            if (zoomPreviewEpochIndx_ >= 0) {
                const int idxSize = static_cast<int>(cursor_.indexes.size());
                for (int i = 0; i < idxSize; ++i) {
                    if (cursor_.indexes[i] == zoomPreviewEpochIndx_) {
                        centerX = i;
                        break;
                    }
                }
            }

            int centerY = qBound(0, canvas().height() / 2, canvas().height() - 1);
            const float distRange = cursor_.distance.to - cursor_.distance.from;
            const bool twoChannelView = cursor_.channel2 != CHANNEL_NONE;
            if (std::isfinite(distRange) && std::abs(distRange) > 1e-6f) {
                if (twoChannelView) {
                    // Keep the same Y mapping as Plot2DAim for 2-channel mode.
                    if (!std::isfinite(zoomPreviewDepth_)) {
                        centerY = 0;
                    }
                    else {
                        const float aimY = static_cast<float>(canvas().height()) * 0.5f
                            - static_cast<float>(canvas().height()) * (zoomPreviewDepth_ / distRange);
                        centerY = qBound(0, qRound(aimY), canvas().height() - 1);
                    }
                }
                else {
                    const float previewDepth = std::isfinite(zoomPreviewDepth_) ? zoomPreviewDepth_ : cursor_.distance.from;
                    const float norm = (previewDepth - cursor_.distance.from) / distRange;
                    centerY = qBound(0, qRound(norm * static_cast<float>(canvas().height() - 1)), canvas().height() - 1);
                }
            }

            const int sourceWidth = qBound(4, zoomPreviewSourceSize_, qMax(4, canvas().width()));
            int sourceHeight = qBound(4, zoomPreviewSourceSize_, qMax(4, canvas().height()));
            if (zoomPreviewReferenceDepthPixels_ > 0 && canvas().height() > 0) {
                const float scaledSource = static_cast<float>(sourceHeight)
                    * static_cast<float>(canvas().height())
                    / static_cast<float>(zoomPreviewReferenceDepthPixels_);
                sourceHeight = qBound(4, qRound(scaledSource), qMax(4, canvas().height()));
            }
            const QRect previewRect(0, 0, w, h);

            painter->fillRect(previewRect, QColor(30, 30, 30, 220));
            QImage previewImage(w, h, QImage::Format_ARGB32_Premultiplied);
            previewImage.fill(Qt::transparent);
            QPainter previewPainter(&previewImage);
            QPointF previewFocus(0.5, 0.5);
            const bool rendered = Plot2D::drawEchogramZoomPreview(&previewPainter,
                                                                  QRect(0, 0, w, h),
                                                                  QPoint(centerX, centerY),
                                                                  sourceWidth,
                                                                  sourceHeight,
                                                                  &previewFocus);
            previewPainter.end();

            if (rendered) {
                if (zoomPreviewFlipY_) {
                    previewImage = previewImage.mirrored(false, true);
                    previewFocus.setY(1.0 - previewFocus.y());
                }
                painter->drawImage(previewRect, previewImage);

                const qreal focusX = qBound<qreal>(0.0, previewFocus.x(), 1.0);
                const qreal focusY = qBound<qreal>(0.0, previewFocus.y(), 1.0);
                const int zoomCenterX = previewRect.left()
                    + qRound(focusX * static_cast<qreal>(qMax(0, previewRect.width() - 1)));
                const int zoomCenterY = previewRect.top()
                    + qRound(focusY * static_cast<qreal>(qMax(0, previewRect.height() - 1)));
                const int crossHalf = qMax(6, qMin(previewRect.width(), previewRect.height()) / 10);
                const int leftArm = qMin(crossHalf, qMax(0, zoomCenterX - previewRect.left()));
                const int rightArm = qMin(crossHalf, qMax(0, previewRect.right() - zoomCenterX));
                const int topArm = qMin(crossHalf, qMax(0, zoomCenterY - previewRect.top()));
                const int bottomArm = qMin(crossHalf, qMax(0, previewRect.bottom() - zoomCenterY));
                painter->setPen(QPen(QColor(255, 255, 255, 220), 2));
                painter->drawLine(zoomCenterX - leftArm, zoomCenterY, zoomCenterX + rightArm, zoomCenterY);
                painter->drawLine(zoomCenterX, zoomCenterY - topArm, zoomCenterX, zoomCenterY + bottomArm);
            }
            else {
                Plot2D::draw(painter);
            }
        }
        else {
            Plot2D::draw(painter);
            if (Plot2D::getIsContactChanged()) {
                emit contactChanged();
            }
        }
    }

    clock_t end = clock();
    int cpu_time_used = ((end-start));
    Q_UNUSED(cpu_time_used);

//    qInfo("r time %d", cpu_time_used);
}

//QSGNode *qPlot2D::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
//{
//    QSGSimpleTextureNode *node = static_cast<QSGSimpleTextureNode *>(oldNode);
//    if (!node) {
//        node = new QSGSimpleTextureNode();

//    }

//    QSGTexture *texture = window()->createTextureFromImage(getImage((int)width(), (int)height()));
////    node->markDirty(QSGNode::DirtyForceUpdate);
//    node->setTexture(texture);
//    node->setRect(boundingRect());
//    return node;
//}

//QSGNode *qPlot2D::updatePaintNode( QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *updatePaintNodeData) {

//    auto node = dynamic_cast<QSGSimpleTextureNode *>(oldNode);

//    if (!node) {
//        node = new QSGSimpleTextureNode();
//    }


//    QQuickWindow* w = window();

//    QSGTexture *texture = window()->createTextureFromImag;

//    node->setOwnsTexture(true);
//    node->setRect(boundingRect());
//    node->markDirty(QSGNode::DirtyForceUpdate);
//    node->setTexture(texture);
//    return node;
//}

void qPlot2D::setPlot(Dataset *dataset) {
    if(dataset == nullptr) { return; }
    m_plot = dataset;
    setDataset(dataset);
    connect(dataset, &Dataset::dataUpdate, this, &qPlot2D::dataUpdate);
//    connect(m_plot, &Dataset::updatedImage, this, [&] { updater(); });
}

void qPlot2D::setDataProcessor(DataProcessor *dataProcessorPtr)
{
    if (!dataProcessorPtr) {
        return;
    }

    Plot2D::setDataProcessorPtr(dataProcessorPtr);
}

void qPlot2D::plotUpdate()
{
    if (!Plot2D::plotEnabled()) {
        return;
    }

    static QMutex mutex;
    if(!mutex.tryLock()) {
//        qInfo("HHHHHHHHHHHHHHHHHHHHHHHHHH==================HHHHHHHHHHHHHHHHHHHHHHHHHH");
        return;
    }
    emit timelinePositionChanged();

    update();

    mutex.unlock();
}

float qPlot2D::getLoupeDepthForEpoch(int epochIndx) const
{
    if (!datasetPtr_ || epochIndx < 0 || epochIndx >= datasetPtr_->size()) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    auto* epoch = datasetPtr_->fromIndex(epochIndx);
    if (!epoch) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    float depth = std::numeric_limits<float>::quiet_NaN();
    if (cursor_.channel1.isValid()) {
        depth = static_cast<float>(epoch->distProccesing(cursor_.channel1));
    }

    if (std::isfinite(depth)) {
        return std::abs(depth);
    }

    // Match Plot2DAim behavior: for 2-channel view without depth, aim the top of preview.
    return cursor_.channel2 == CHANNEL_NONE ? 0.0f : std::numeric_limits<float>::quiet_NaN();
}

bool qPlot2D::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);

    if (event->type() == EpochSelected3d) {
        auto epochEvent = static_cast<EpochEvent*>(event);
        //qDebug() << QString("[Plot 2d]: catched event from 3d view (epoch index is %1)").arg(epochEvent->epochIndex());
        setAimEpochEventState(true);
        setTimelinePositionByEpoch(epochEvent->epochIndex());
    }
    return false;
}

void qPlot2D::sendSyncEvent(int epoch_index, QEvent::Type eventType)
{
    //qDebug() << "qPlot2D::sendSyncEvent: epoch_index: " << epoch_index;
    if (eventType == EpochSelected2d) {
        cursor_.selectEpochIndx = -1;
    }

    auto epochEvent = new EpochEvent(eventType, datasetPtr_->fromIndex(epoch_index), epoch_index, DatasetChannel(cursor_.channel1, cursor_.subChannel1));
    QCoreApplication::postEvent(this, epochEvent);
}

void qPlot2D::horScrollEvent(int delta) {
    cursor_.selectEpochIndx = -1;

    if(_isHorizontal) {
        scrollPosition(-delta);
    } else {
        scrollPosition(delta);
    }
}

void qPlot2D::verZoomEvent(int delta) {
//    if(m_plot != nullptr) {
//        m_plot->verZoom(delta);
//    }

    zoomDistance(delta);
}

void qPlot2D::verScrollEvent(int delta) {
    scrollDistance(delta);
}

void qPlot2D::plotMouseTool(int mode) {
    setMouseTool((MouseTool)mode);
}

bool qPlot2D::setContact(int indx, const QString& text)
{
    return Plot2D::setContact(indx, text);
}

bool qPlot2D::setActiveContact(int indx)
{
    return Plot2D::setActiveContact(indx);
}

bool qPlot2D::deleteContact(int indx)
{
    return Plot2D::deleteContact(indx);
}

void qPlot2D::updateContact()
{
    Plot2D::updateContact();
}

void qPlot2D::setPlotEnabled(bool state)
{
    Plot2D::setPlotEnabled(state);
}

void qPlot2D::mosaicLOffsetChanged(float val)
{
    Plot2D::setMosaicLOffset(val);
}

void qPlot2D::mosaicROffsetChanged(float val)
{
    Plot2D::setMosaicROffset(val);
}

float qPlot2D::getLowEchogramLevel() const
{
    return Plot2D::getEchogramLowLevel();
}

float qPlot2D::getHighEchogramLevel() const
{
    return Plot2D::getEchogramHighLevel();
}

int qPlot2D::getThemeId() const
{
    return Plot2D::getThemeId();
}

void qPlot2D::doDistProcessing(int preset, int window_size, float vertical_gap, float range_min, float range_max, float gain_slope, float threshold, float offsetx, float offsety, float offsetz, bool manual) {
    if (datasetPtr_ != nullptr) {
        if (auto btpPtr = datasetPtr_->getBottomTrackParamPtr(); btpPtr) {
            btpPtr->preset = static_cast<BottomTrackPreset>(preset);
            btpPtr->gainSlope = gain_slope;
            btpPtr->threshold = threshold;
            btpPtr->windowSize = window_size;
            btpPtr->verticalGap = vertical_gap;
            btpPtr->minDistance = range_min;
            btpPtr->maxDistance = range_max;
            btpPtr->indexFrom = 0;
            btpPtr->indexTo = datasetPtr_->size();
            btpPtr->offset.x = offsetx;
            btpPtr->offset.y = offsety;
            btpPtr->offset.z = offsetz;

            QMetaObject::invokeMethod(dataProcessorPtr_, "bottomTrackProcessing", Qt::QueuedConnection,
                                      Q_ARG(DatasetChannel, DatasetChannel(cursor_.channel1, cursor_.subChannel1)),
                                      Q_ARG(DatasetChannel, DatasetChannel(cursor_.channel2, cursor_.subChannel2)),
                                      Q_ARG(BottomTrackParam, *btpPtr),
                                      Q_ARG(bool, manual),
                                      Q_ARG(bool, true)/*redraw all*/);
        }
    }
    plotUpdate();
}

void qPlot2D::refreshDistParams(int preset, int windowSize, float verticalGap, float rangeMin, float rangeMax, float gainSlope, float threshold, float offsetX, float offsetY, float offsetZ)
{
    auto btPRefreshFunc = [this, preset, windowSize, verticalGap, rangeMin, rangeMax, gainSlope, threshold, offsetX, offsetY, offsetZ]() {
        if (datasetPtr_) {
            if (auto btpPtr =datasetPtr_->getBottomTrackParamPtr(); btpPtr) {
                btpPtr->preset = static_cast<BottomTrackPreset>(preset);
                btpPtr->gainSlope = gainSlope;
                btpPtr->threshold = threshold;
                btpPtr->windowSize = windowSize;
                btpPtr->verticalGap = verticalGap;
                btpPtr->minDistance = rangeMin;
                btpPtr->maxDistance = rangeMax;
                btpPtr->indexFrom = 0;
                btpPtr->indexTo = datasetPtr_->size();
                btpPtr->offset.x = offsetX;
                btpPtr->offset.y = offsetY;
                btpPtr->offset.z = offsetZ;
            }
        }
    };

    if (!datasetPtr_) {
        pendingBtpLambda_ = btPRefreshFunc;
    } else {
        btPRefreshFunc();
    }
}

void qPlot2D::setPreset(int value)
{
    if (datasetPtr_) {
        if (auto* btpPtr =datasetPtr_->getBottomTrackParamPtr(); btpPtr) {
            btpPtr->preset = static_cast<BottomTrackPreset>(value);
        }
    }
}

void qPlot2D::setWindowSize(int value)
{
    if (datasetPtr_) {
        if (auto* btpPtr =datasetPtr_->getBottomTrackParamPtr(); btpPtr) {
            btpPtr->windowSize = value;
        }
    }
}

void qPlot2D::setVerticalGap(float value)
{
    if (datasetPtr_) {
        if (auto* btpPtr =datasetPtr_->getBottomTrackParamPtr(); btpPtr) {
            btpPtr->verticalGap = value;
        }
    }
}

void qPlot2D::setRangeMin(float value)
{
    if (datasetPtr_) {
        if (auto* btpPtr =datasetPtr_->getBottomTrackParamPtr(); btpPtr) {
            btpPtr->minDistance = value;
        }
    }
}

void qPlot2D::setRangeMax(float value)
{
    if (datasetPtr_) {
        if (auto* btpPtr =datasetPtr_->getBottomTrackParamPtr(); btpPtr) {
            btpPtr->maxDistance = value;
        }
    }
}

void qPlot2D::setGainSlope(float value)
{
    if (datasetPtr_) {
        if (auto* btpPtr =datasetPtr_->getBottomTrackParamPtr(); btpPtr) {
            btpPtr->gainSlope = value;
        }
    }
}

void qPlot2D::setThreshold(float value)
{
    if (datasetPtr_) {
        if (auto* btpPtr =datasetPtr_->getBottomTrackParamPtr(); btpPtr) {
            btpPtr->threshold = value;
        }
    }
}

void qPlot2D::setOffsetX(float value)
{
    if (datasetPtr_) {
        if (auto* btpPtr =datasetPtr_->getBottomTrackParamPtr(); btpPtr) {
            btpPtr->offset.x = value;
        }
    }
}

void qPlot2D::setOffsetY(float value)
{
    if (datasetPtr_) {
        if (auto* btpPtr =datasetPtr_->getBottomTrackParamPtr(); btpPtr) {
            btpPtr->offset.y = value;
        }
    }
}

void qPlot2D::setOffsetZ(float value)
{
    if (datasetPtr_) {
        if (auto* btpPtr =datasetPtr_->getBottomTrackParamPtr(); btpPtr) {
            btpPtr->offset.z = value;
        }
    }
}

void qPlot2D::plotMousePosition(int x, int y, bool isSync)
{
    setAimEpochEventState(false);
    if(_isHorizontal) {
        setMousePosition(x, y, isSync);
    } else {
        if(x >=0 && y >= 0) {
            setMousePosition(height() - y, x, isSync);
        } else {
            setMousePosition(-1, -1, isSync);
        }

    }
}

void qPlot2D::simplePlotMousePosition(int x, int y) {
    Plot2D::setAimEpochEventState(false);

    if(_isHorizontal) {
        Plot2D::simpleSetMousePosition(x, y);
    } 
    else {
        if(x >=0 && y >= 0) {
            Plot2D::simpleSetMousePosition(height() - y, x);
        }
        else {
            Plot2D::simpleSetMousePosition(-1, -1);
        }
    }
}

void qPlot2D::onCursorMoved(int x, int y)
{
    Plot2D::onCursorMoved(x, y);
}

void qPlot2D::timerUpdater() {
    if(m_needUpdate) {
        m_needUpdate = false;
       update();
    }
}

void qPlot2D::updater() {
    m_needUpdate = true;
}

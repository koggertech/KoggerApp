#include "waterfall.h"
#include <QPixmap>
#include <QPainter>
#include <QSGSimpleTextureNode>
#include <QQuickWindow>

#include <QMutex>
#include <core.h>
#include <epochevent.h>

qPlot2D::qPlot2D(QQuickItem* parent)
    : QQuickPaintedItem(parent)
    , m_updateTimer(new QTimer(this))
{
//    setRenderTarget(QQuickPaintedItem::FramebufferObject);
//    connect(m_updateTimer, &QTimer::timeout, this, [&] { update(); });
    m_updateTimer->start(30);
    setFlag(ItemHasContents);
    setAcceptedMouseButtons(Qt::AllButtons);
//    setFillColor(QColor(255, 255, 255));

    _isHorizontal = false;
}

#include <time.h>

void qPlot2D::paint(QPainter *painter) {
    clock_t start = clock();

    if(m_plot != nullptr && painter != nullptr) {
         getImage((int)width(), (int)height(), painter);
    }

    clock_t end = clock();
    int cpu_time_used = ((end-start));

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

void qPlot2D::plotUpdate() {
    static QMutex mutex;
    if(!mutex.tryLock()) {
//        qInfo("HHHHHHHHHHHHHHHHHHHHHHHHHH==================HHHHHHHHHHHHHHHHHHHHHHHHHH");
        return;
    }
//    emit timelinePositionChanged();

    update();

    mutex.unlock();
}

bool qPlot2D::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == EpochSelected3d) {
        auto epochEvent = static_cast<EpochEvent*>(event);
        qDebug() << QString("[Plot 2d]: catched event from 3d view (epoch index is %1)").arg(epochEvent->epochIndex());
    }
    return false;
}

void qPlot2D::sendSyncEvent(int epoch_index) {
    // qDebug() << "Cursor epoch" << epoch_index;
    auto epochEvent = new EpochEvent(EpochSelected2d, _dataset->fromIndex(epoch_index), epoch_index, _cursor.channel1);
    QCoreApplication::postEvent(this, epochEvent);
}

void qPlot2D::horScrollEvent(int delta) {
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


void qPlot2D::doDistProcessing(int preset, int window_size, float vertical_gap, float range_min, float range_max, float gain_slope, float threshold, float offsetx, float offsety, float offsetz) {
    if(_dataset != nullptr) {
        _bottomTrackParam.preset = (BottomTrackPreset)preset;
        _bottomTrackParam.gainSlope = gain_slope;
        _bottomTrackParam.threshold = threshold;
        _bottomTrackParam.windowSize = window_size;
        _bottomTrackParam.verticalGap = vertical_gap;
        _bottomTrackParam.minDistance = range_min;
        _bottomTrackParam.maxDistance = range_max;
        _bottomTrackParam.indexFrom = 0;
        _bottomTrackParam.indexTo = _dataset->size();
        _bottomTrackParam.offset.x = offsetx;
        _bottomTrackParam.offset.y = offsety;
        _bottomTrackParam.offset.z = offsetz;
        _dataset->bottomTrackProcessing(_cursor.channel1, _cursor.channel2, _bottomTrackParam);
    }
    plotUpdate();
}

void qPlot2D::plotMousePosition(int x, int y) {
    if(_isHorizontal) {
        setMousePosition(x, y);
    } else {
        if(x >=0 && y >= 0) {
            setMousePosition(height() - y, width() - x);
        } else {
            setMousePosition(-1, -1);
        }

    }
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

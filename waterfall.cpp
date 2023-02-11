#include "waterfall.h"
#include <QPixmap>
#include <QPainter>

WaterFall::WaterFall(QQuickItem* parent)
    : QQuickPaintedItem(parent)
    , m_updateTimer(new QTimer(this))
{
    connect(m_updateTimer, &QTimer::timeout, this, [&] { timerUpdater(); });
    m_updateTimer->start(30);
    setAcceptedMouseButtons(Qt::AllButtons);

    _isHorizontal = false;
}

void WaterFall::paint(QPainter *painter){
    static QPixmap pix;
    if(m_plot != nullptr && painter != nullptr) {
        if(_isHorizontal) {
            pix = QPixmap::fromImage(m_plot->getImage({(int)width(), (int)height()}), Qt::NoFormatConversion);
        } else {
            pix = QPixmap::fromImage(m_plot->getImage({(int)height(), (int)width()}).transformed(QMatrix().rotate(-90.0), Qt::FastTransformation).mirrored(true, false), Qt::NoFormatConversion);
        }

        painter->drawPixmap(0, 0, pix);
    }
}

void WaterFall::setPlot(PlotCash *plot) {
    if(plot == nullptr) { return; }
    m_plot = plot;
    connect(m_plot, &PlotCash::updatedImage, this, [&] { updater(); });
}

void WaterFall::horScrollEvent(int delta) {
    if(m_plot != nullptr) {
        if(_isHorizontal) {
            m_plot->scrollTimeline(delta);
        } else {
            m_plot->scrollTimeline(-delta);
        }

    }
}

void WaterFall::verZoomEvent(int delta) {
    if(m_plot != nullptr) {
        m_plot->verZoom(delta);
    }
}

void WaterFall::verScrollEvent(int delta) {
    if(m_plot != nullptr) {
        m_plot->verScroll(delta);
    }
}

void WaterFall::setMouseMode(int mode) {
    if(m_plot != nullptr) {
        m_plot->setMouseMode(mode);
    }
}

void WaterFall::setMouse(int x, int y) {
    if(m_plot != nullptr) {
        if(_isHorizontal) {
            m_plot->setMouse(x, y);
        } else {
            if(x >=0 && y >= 0) {
                m_plot->setMouse(height() - y, width() - x);
            } else {
                m_plot->setMouse(-1, -1);
            }

        }
    }
}

void WaterFall::timerUpdater() {
    if(m_needUpdate) {
        m_needUpdate = false;
       update();
    }
}

void WaterFall::updater() {
    m_needUpdate = true;
}

#include "waterfall.h"
#include <QPixmap>
#include <QPainter>

WaterFall::WaterFall(QQuickItem* parent)
    : QQuickPaintedItem(parent)
    , m_updateTimer(new QTimer(this))
{
    connect(m_updateTimer, &QTimer::timeout, this, [&] { timerUpdater(); });
    m_updateTimer->start(30);
}

void WaterFall::paint(QPainter *painter){
    static QPixmap pix;
    if(m_plot != nullptr && painter != nullptr) {
        pix = QPixmap::fromImage(m_plot->getImage({(int)width(), (int)height()}), Qt::NoFormatConversion);
        painter->drawPixmap(0, 0, pix);
    }
}

void WaterFall::setPlot(PlotCash *plot) {
    if(plot == nullptr) { return; }
    m_plot = plot;
    connect(m_plot, &PlotCash::updatedImage, this, [&] { updater(); });
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

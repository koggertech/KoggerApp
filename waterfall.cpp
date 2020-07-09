#include "waterfall.h"
#include <QPixmap>
#include <QPainter>

WaterFall::WaterFall(QQuickItem* parent)
    : QQuickPaintedItem(parent)
    , m_updateTimer(new QTimer(this))
{
    connect(m_updateTimer, &QTimer::timeout, this, [&] { update(); });
    m_updateTimer->start(30);
}

void WaterFall::paint(QPainter *painter){
    static QPixmap pix;
    if(m_plot != nullptr) {
        pix = QPixmap::fromImage(m_plot->getImage({(int)width(), (int)height()}), Qt::NoFormatConversion);
        painter->drawPixmap(0, 0, pix);
    }
}

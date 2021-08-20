#ifndef WATERFALL_H
#define WATERFALL_H

#include <QImage>
#include <QQuickPaintedItem>
#include <QObject>
#include <plotcash.h>
#include <QTimer>

class WaterFall : public QQuickPaintedItem
{
    Q_OBJECT
public:
    WaterFall(QQuickItem* parent = nullptr);
    virtual void paint(QPainter *painter);

    void setPlot(PlotCash* plot);

protected:
    PlotCash* m_plot = nullptr;
    QTimer* m_updateTimer;
    bool m_needUpdate = true;

protected slots:
    void timerUpdater();

public slots:
    void updater();
    void horScrollEvent(int delta);
    void verZoomEvent(int delta);
    void verScrollEvent(int delta);

};

#endif // WATERFALL_H

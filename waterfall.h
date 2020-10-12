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
private:
    PlotCash* m_plot = nullptr;
    QTimer* m_updateTimer;
    bool m_needUpdate = true;

private slots:
    void timerUpdater();

public slots:
    void updater();

};

#endif // WATERFALL_H

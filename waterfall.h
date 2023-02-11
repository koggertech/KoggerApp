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
    Q_PROPERTY(bool horizontal READ isHorizontal() WRITE setHorizontal)

    WaterFall(QQuickItem* parent = nullptr);
    virtual void paint(QPainter *painter);

    void setPlot(PlotCash* plot);
    bool isHorizontal() { return _isHorizontal; }
    void setHorizontal(bool is_horizontal) { _isHorizontal = is_horizontal;  updater(); }

protected:
    PlotCash* m_plot = nullptr;
    QTimer* m_updateTimer;
    bool m_needUpdate = true;
    bool _isHorizontal = true;

protected slots:
    void timerUpdater();

public slots:
    void updater();
    void horScrollEvent(int delta);
    void verZoomEvent(int delta);
    void verScrollEvent(int delta);
    void setMouse(int x, int y);
    void setMouseMode(int mode);
};

#endif // WATERFALL_H

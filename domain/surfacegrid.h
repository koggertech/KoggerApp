#ifndef SURFACEGRID_H
#define SURFACEGRID_H

#include <QObject>

#include <displayedobject.h>

class SurfaceGrid : public DisplayedObject
{
    Q_OBJECT

public:
    explicit SurfaceGrid(QObject *parent = nullptr);

signals:

};

#endif // SURFACEGRID_H

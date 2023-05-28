#ifndef MARKUPGRID_H
#define MARKUPGRID_H

#include <QSet>

#include <displayedobject.h>
#include <gridgenerator.h>

class MarkupGrid : public DisplayedObject
{
public:
    MarkupGrid();

    void setSize(QVector3D topLeft, float width, float height, float dist);
    void setSize(QVector3D topLeft, QVector3D bottomRight, float dist);
};

#endif // MARKUPGRID_H

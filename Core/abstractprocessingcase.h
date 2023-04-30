#ifndef ABSTRACTPROCESSINGCASE_H
#define ABSTRACTPROCESSINGCASE_H

#include <QVector>
#include <QVector3D>

#include "vertexobject.h"
#include "Point3D.h"
#include "sceneparams.h"
#include "boundarydetector.h"

class AbstractProcessingCase
{
public:
    virtual VertexObject process(const QVector <QVector3D>& bottomTrack, SceneParams params) = 0;

    VertexObject contourVertexObject() const {return mContourVertexObject;};

protected:

    VertexObject mContourVertexObject;
};

#endif // ABSTRACTPROCESSINGCASE_H

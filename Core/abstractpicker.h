#ifndef ABSTRACTPICKER_H
#define ABSTRACTPICKER_H

#include <memory>

#include <QVector3D>

class AbstractPicker
{
public:

    //virtual std::shared_ptr <VertexObject> pick(std::shared_ptr <VertexObject> object) = 0;

    virtual QString pickingMethod() = 0;

    QVector3D lastIntersectionPoint() const {
        return mLastIntersectionPoint;
    };

protected:

    QVector3D mLastIntersectionPoint;
};

#endif // ABSTRACTPICKER_H

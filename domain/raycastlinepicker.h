#ifndef RAYCASTLINEPICKER_H
#define RAYCASTLINEPICKER_H

#include <QDebug>

#include <abstractpicker.h>

class RayCastLinePicker : public AbstractPicker
{
public:
    RayCastLinePicker(const QVector3D& origin, const QVector3D& dir);

    virtual VertexObject pick(const VertexObject& object) override;
    virtual QString pickingMethod() override;

private:

    QVector3D mOrigin;
    QVector3D mDir;
};

#endif // RAYCASTLINEPICKER_H

#ifndef RAYCASTLINEPICKER_H
#define RAYCASTLINEPICKER_H

#include <QDebug>

#include <abstractpicker.h>

class RayCastLinePicker : public AbstractPicker
{
public:
    RayCastLinePicker(const QVector3D& origin, const QVector3D& dir);

    //virtual std::shared_ptr <VertexObject> pick(std::shared_ptr <VertexObject> sourceObject) override;
    virtual QString pickingMethod() override;

private:

    QVector3D mOrigin;
    QVector3D mDir;
};

#endif // RAYCASTLINEPICKER_H

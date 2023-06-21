#ifndef RAYCASTPICKERFACTORY_H
#define RAYCASTPICKERFACTORY_H

#include <abstractpickerfactory.h>
#include <raycastpointpicker.h>

class RayCastPickerFactory : AbstractPickerFactory
{
public:

    RayCastPickerFactory() = delete;
    RayCastPickerFactory(const QVector3D& origin, const QVector3D& dir);

    std::shared_ptr <AbstractPicker> createPolygonPicker() override;
    std::shared_ptr <AbstractPicker> createPointPicker() override;

private:

    std::pair <QVector3D, QVector3D> createRay();

private:

    QVector3D mOrigin;
    QVector3D mDir;
};

#endif // RAYCASTPICKERFACTORY_H

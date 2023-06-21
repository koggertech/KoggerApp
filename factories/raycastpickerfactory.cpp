#include "raycastpickerfactory.h"

RayCastPickerFactory::RayCastPickerFactory(const QVector3D& origin, const QVector3D& dir)
: mOrigin(origin)
, mDir(dir)
{}

std::shared_ptr<AbstractPicker> RayCastPickerFactory::createPolygonPicker()
{
    std::shared_ptr <AbstractPicker> picker = std::make_shared <RayCastPolygonPicker> (mOrigin, mDir);

    return picker;
}

std::shared_ptr<AbstractPicker> RayCastPickerFactory::createPointPicker()
{
    std::shared_ptr <AbstractPicker> picker = std::make_shared <RayCastPointPicker> (mOrigin, mDir);

    return picker;
}

#ifndef ABSTRACTPICKERFACTORY_H
#define ABSTRACTPICKERFACTORY_H

#include <memory>

#include <abstractpicker.h>

class AbstractPickerFactory
{
public:

    virtual std::shared_ptr <AbstractPicker> createPolygonPicker() = 0;
    virtual std::shared_ptr <AbstractPicker> createPointPicker() = 0;
};

#endif // ABSTRACTPICKERFACTORY_H

#include "surface.h"

Surface::Surface()
: DisplayedObject()
{

}

void Surface::setBasicCalculationMethod(QString method)
{
    mBasicCalculationMethod = method;
}

void Surface::setSmoothingMethod(QString method)
{
    mSmoothingMethod = method;
}

QString Surface::basicCalculationMethod() const
{
    return mBasicCalculationMethod;
}

QString Surface::smoothingMethod() const
{
    return mSmoothingMethod;
}

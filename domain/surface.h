#ifndef SURFACE_H
#define SURFACE_H

#include <displayedobject.h>

#include <constants.h>

class Surface : public DisplayedObject
{
public:
    Surface();

    void setBasicCalculationMethod(QString method);
    void setSmoothingMethod(QString method);
    QString basicCalculationMethod() const;
    QString smoothingMethod() const;

private:

    QString mBasicCalculationMethod = "TIN";
    QString mSmoothingMethod = "None";
};

#endif // SURFACE_H

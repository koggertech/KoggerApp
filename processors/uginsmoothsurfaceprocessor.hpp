//###################################################################
//! Kogger
//!
//! @author Харламенко И.В.
//! @date 2023
//###################################################################

#pragma once

#include <QDebug>

#include "abstractsurfaceprocessor.hpp"
#include "Interpolator.h"

class UginSmoothSurfaceProcessor : public AbstractSurfaceProcessor
{
    //! @brief Выполняет сглаживание поверхности.
    //! @note Является переопределенным методом родительского класса
    VertexObject process(const QVector <QVector3D>& points) override;

    void setParam(Param p, QVariant value) override;

private:

    int mInterpolationLevel = 1;
};

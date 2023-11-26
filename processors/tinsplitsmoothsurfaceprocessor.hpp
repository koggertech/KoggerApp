//###################################################################
//! Kogger
//!
//! @author Харламенко И.В.
//! @date 2023
//###################################################################

#include "abstractsurfaceprocessor.hpp"
#include "Triangle.h"
#include "gridgenerator.h"

#include <queue>

#include <QDebug>

class TinSplitSmoothSurfaceProcessor : public AbstractSurfaceProcessor
{

public:

    //! @brief Выполняет базовый расчет поверхности.
    //! @note Является чистым виртуальным методом и обязателен
    //! к переопределению классами - наследниками
    VertexObject process(const QVector <QVector3D>& points) override;

    void setParam(Param p, QVariant value) override;

private:

    double mTriangleEdgeMinLength = 4.0f;

    void calculateZ(Triangle <float>& t, Point3D <float>& p);

    void proc(Triangle <float>& t, float length, std::vector <Triangle<float>>& grid);

};

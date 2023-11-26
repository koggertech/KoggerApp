//###################################################################
//! Kogger
//!
//! @author Харламенко И.В.
//! @date 2023
//###################################################################

#pragma once

#include <QVector>
#include <QVector3D>
#include <QVariant>

#include <vertexobject.h>

/**
 * @brief Абстрактный базовый класс модуля базового расчета поверхности.
 */
class AbstractSurfaceProcessor
{
public:

    enum class Param
    {
        triangleEdgeLengthLimit = 0,
        interpolationLevel = 1,
        triangleEdgeMinLength = 2,
        unknown = 3
    };

    virtual ~AbstractSurfaceProcessor();

    //! @brief Выполняет базовый расчет поверхности.
    //! @return Экземпляр класса вершинного объекта
    //! @note Является чистым виртуальным методом и обязателен
    //! к переопределению классами - наследниками
    virtual VertexObject process(const QVector <QVector3D>& points) = 0;

    virtual void setParam(Param p, QVariant value) = 0;

};

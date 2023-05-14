//###################################################################
//! Kogger
//!
//! @author Харламенко И.В.
//! @date 2023
//###################################################################

#pragma once

#include <memory>

#include <QVector>
#include <QVector3D>

#include "abstractsurfaceprocessor.hpp"

/**
 * @brief Абстрактный базовый класс объекта - создателя конкретных модулей
 * расчета поверхности.
 * @note Является базовым классом как для создателей объектов базового расчета,
 * так и для создателей объектов сглаживания поверхности
 */
class AbstractSurfaceProcessorFactory
{
public:

    virtual ~AbstractSurfaceProcessorFactory();

    //! @brief Создает экземпляр модуля расчета поверхности.
    //! @return Указатель на экземпляр модуля расчета поверхности.
    //! @param type - метод расчета поверхности
    //! @note Является чистым виртуальным методом и обязателен
    //! к переопределению классами - наследниками
    virtual AbstractSurfaceProcessor* createProcessor(const QString& method) = 0;
};

//###################################################################
//! Kogger
//!
//! @author Харламенко И.В.
//! @date 2023
//###################################################################

#pragma once

#include "abstractsurfaceprocessorfactory.hpp"
#include "tinbasicsurfaceprocessor.hpp"

#include <constants.h>

class BasicSurfaceProcessorFactory : public AbstractSurfaceProcessorFactory
{
public:

    enum class BasicProcessingMethod : uint8_t
    {
        TIN = 0
    };

    //! @brief Создает экземпляр модуль расчета поверхности.
    //! @param method - метод расчета поверхности
    AbstractSurfaceProcessor* createProcessor(const QString& method) override
    {
        AbstractSurfaceProcessor* pProcessor = nullptr;

        if(method == CALCULATION_METHOD_TIN){
            pProcessor = new TinBasicSurfaceProcessor;
        }

        return pProcessor;
    }

};

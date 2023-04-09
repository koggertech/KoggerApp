//###################################################################
//! Kogger
//!
//! @author Харламенко И.В.
//! @date 2023
//###################################################################

#pragma once

#include "abstractsurfaceprocessorfactory.hpp"
#include "uginsmoothsurfaceprocessor.hpp"
#include "tinsplitsmoothsurfaceprocessor.hpp"

class SmoothSurfaceProcessorFactory : public AbstractSurfaceProcessorFactory
{
public:

    enum class SmoothMethod : uint8_t
    {
        UGIN = 0
    };

    //! @brief Создает экземпляр модуль расчета поверхности.
    //! @param method - метод расчета поверхности
    AbstractSurfaceProcessor* createProcessor(const QString& method) override
    {
        AbstractSurfaceProcessor* pProcessor = nullptr;

        if(method == "UGIN"){
            pProcessor = new UginSmoothSurfaceProcessor;
        }else if (method == "Triangle split"){
            pProcessor = new TinSplitSmoothSurfaceProcessor;
        }

        return pProcessor;
    }
};

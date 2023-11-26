#ifndef PROCESSINGCASEFACTORY_H
#define PROCESSINGCASEFACTORY_H

#include "abstractprocessingcase.h"
#include "tinwithbarycentriccase.h"
#include "onlytincase.h"

class ProcessingCaseFactory
{
public:

    AbstractProcessingCase *createCase(SceneParams params)
    {
        AbstractProcessingCase* pCase = nullptr;

        if (params.basicProcessingMethod() == CALCULATION_METHOD_TIN &&
            params.smoothingMethod() == SMOOTHING_METHOD_BARYCENTRIC){
            return new TinWithBarycentricCase;
        }

        if (params.basicProcessingMethod() == CALCULATION_METHOD_TIN &&
                params.smoothingMethod() == SMOOTHING_METHOD_NONE){
            return new OnlyTinCase;
        }

        return pCase;
    }
};

#endif // PROCESSINGCASEFACTORY_H

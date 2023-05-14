#ifndef SCENEPARAMS_H
#define SCENEPARAMS_H

#include <QString>

#include "constants.h"

class SceneParams
{
public:

    void setBasicProcessingMethod(QString method) {mBasicProcessingMethod = method;};
    void setSmoothingMethod(QString method) {mSmoothingMethod = method;};
    void setGridType(QString type) {mGridType = type;};
    void setTriangulationEdgeLengthLimit(double limit) {mTiangulationEdgeLengthLimit = limit;};
    void setGridCellSideSize(double size) {mGridCellSideSize = size;};

    QString basicProcessingMethod() const {return mBasicProcessingMethod;};
    QString smoothingMethod() const {return mSmoothingMethod;};
    QString gridType() const {return mGridType;};
    float triangulationEdgeLengthLimit() const {return mTiangulationEdgeLengthLimit;};
    float gridCellSideSize() const {return mGridCellSideSize;};

private:

    QString mBasicProcessingMethod = CALCULATION_METHOD_TIN;
    QString mSmoothingMethod = SMOOTHING_METHOD_BARYCENTRIC;
    QString mGridType = GRID_TYPE_QUAD;
    float mTiangulationEdgeLengthLimit = -1.0f;
    float mGridCellSideSize = 1.0f;
};

#endif // SCENEPARAMS_H

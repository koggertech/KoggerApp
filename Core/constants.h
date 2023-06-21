#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

static const QString CALCULATION_METHOD_TIN = "TIN";

static const QString SMOOTHING_METHOD_NONE  = "None";
static const QString SMOOTHING_METHOD_UGIN  = "UGIN";
static const QString SMOOTHING_METHOD_BARYCENTRIC  = "Barycentric";

static const QString GRID_TYPE_QUAD = "Quad";
static const QString GRID_TYPE_TRIANGLE = "Triangle";

static const QString BT_FILTRATION_METHOD_NEAREST_POINT = "Nearest point";
static const QString BT_FILTRATION_METHOD_MAX_POINTS = "Max points";

static const QString PICKING_METHOD_NONE = "None";
static const QString PICKING_METHOD_POINT = "Point";
static const QString PICKING_METHOD_POLYGON = "Polygon";

#endif // CONSTANTS_H

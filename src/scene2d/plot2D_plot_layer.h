#pragma once

#include "dataset.h"


class Plot2D;
class PlotLayer {
public:
    PlotLayer() = default;
    virtual ~PlotLayer() = default;

    bool isVisible() const { return isVisible_; }
    void setVisible(bool visible) { isVisible_ = visible; }

    virtual bool draw(Plot2D* parent, Dataset* dataset) = 0;

protected:
    bool isVisible_ = false;
};

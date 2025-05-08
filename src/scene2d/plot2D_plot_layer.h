#pragma once

#include "dataset.h"


class Plot2D;
class PlotLayer {
public:
    PlotLayer() = default;
    virtual ~PlotLayer() = default;

    bool isVisible() const { return isVisible_; }
    bool isFillWidth() const { return fillWidth_; }

    void setVisible(bool visible) { isVisible_ = visible; }
    void setFillWidth(bool state) { fillWidth_ = state; }

    virtual bool draw(Plot2D* parent, Dataset* dataset) = 0;

protected:
    bool isVisible_ = false;

private:
    bool fillWidth_ = false;
};

#pragma once

#include "plot2D_plot_layer.h"


class Plot2DContact : public PlotLayer {
public:
    Plot2DContact() = default;
    bool draw(Plot2D *parent, Dataset *dataset);
    void setMousePos(int x, int y);

    bool isChanged();

    QString getInfo();
    void setInfo(const QString& info);
    bool getVisible();
    void setVisible(bool visible);
    QPoint getPosition();
    int getIndx();
    double getLat();
    double getLon();
    double getDepth();
    void setIsHorizontal(bool state) { isHorizontal_ = state; };

private:
    int lineWidth_ = 1;
    QColor lineColor_ = { 255, 255, 255, 255 };

    int mouseX_ = -1;
    int mouseY_ = -1;
    int indx_ = -1;
    QPoint position_;
    QString info_;
    bool visible_ = false;
    bool visibleChanged_ = false;
    double lat_ = 0.0;
    double lon_ = 0.0;
    double depth_ = 0.0;
    bool isHorizontal_ = true;

    void setVisibleContact(bool val);
};

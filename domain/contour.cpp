#include "contour.h"

Contour::Contour()
{

}

float Contour::lineWidth() const
{
    return mLineWidth;
}

bool Contour::keyPointsVisible() const
{
    return mKeyPointsVisible;
}

QColor Contour::keyPointsRgbColor() const
{
    return mKeyPointsColor;
}

QVector4D Contour::keyPointsColor() const
{
    float rgb_max = 255.0f;

    auto r = mKeyPointsColor.red();
    auto g = mKeyPointsColor.green();
    auto b = mKeyPointsColor.blue();
    auto a = mKeyPointsColor.alpha();

    QVector4D vec{static_cast <float>(r) / rgb_max
                 ,static_cast <float>(g) / rgb_max
                 ,static_cast <float>(b) / rgb_max
                 ,static_cast <float>(a) / rgb_max};

    return vec;
}

void Contour::setLineWidth(float width)
{
    if (width < 1.0f){
        mLineWidth = 1.0f;
    }

    mLineWidth = width;
}

void Contour::setKeyPointsVisible(bool visible)
{
    mKeyPointsVisible = visible;
}

void Contour::setKeyPointsColor(QColor color)
{
    mKeyPointsColor = color;
}

#include "contour.h"

Contour::Contour(QObject* parent)
: SceneObject(parent)
{}

Contour::~Contour()
{}

bool Contour::keyPointsVisible() const
{
    return m_keyPointsVisible;
}

QColor Contour::keyPointsRgbColor() const
{
    return m_keyPointsColor;
}

QVector4D Contour::keyPointsColor() const
{
    float rgb_max = 255.0f;

    auto r = m_keyPointsColor.red();
    auto g = m_keyPointsColor.green();
    auto b = m_keyPointsColor.blue();
    auto a = m_keyPointsColor.alpha();

    QVector4D vec{static_cast <float>(r) / rgb_max
                 ,static_cast <float>(g) / rgb_max
                 ,static_cast <float>(b) / rgb_max
                 ,static_cast <float>(a) / rgb_max};

    return vec;
}

void Contour::setKeyPointsVisible(bool visible)
{
    m_keyPointsVisible = visible;
}

void Contour::setKeyPointsColor(QColor color)
{
    m_keyPointsColor = color;
}

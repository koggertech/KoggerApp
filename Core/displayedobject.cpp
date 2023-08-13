#include "displayedobject.h"

DisplayedObject::DisplayedObject(QObject* parent)
: VertexObject(parent)
{

}

DisplayedObject::DisplayedObject(const int type, QObject* parent)
: VertexObject(type, parent)
{
}

DisplayedObject::DisplayedObject(const int type, const QVector<QVector3D> &data, QObject* parent)
: VertexObject(type, data, parent)
{

}

DisplayedObject::~DisplayedObject()
{
}

void DisplayedObject::setVisible(bool isVisible)
{
    if (mIsVisible == isVisible)
        return;

    mIsVisible = isVisible;

    Q_EMIT visibilityChanged(mIsVisible);
}

void DisplayedObject::setColor(QColor color)
{
    if (mColor == color)
        return;

    mColor = color;

    Q_EMIT colorChanged(color);
}

bool DisplayedObject::isVisible() const
{
    return mIsVisible;
}

QColor DisplayedObject::rgbColor() const
{
    return mColor;
}

QVector4D DisplayedObject::color4d() const
{
    float rgb_max = 255.0f;

    auto r = mColor.red();
    auto g = mColor.green();
    auto b = mColor.blue();
    auto a = mColor.alpha();

    QVector4D vec{static_cast <float>(r) / rgb_max
                 ,static_cast <float>(g) / rgb_max
                 ,static_cast <float>(b) / rgb_max
                 ,static_cast <float>(a) / rgb_max};

    return vec;
}

QColor DisplayedObject::color()
{
    return mColor;
}

float DisplayedObject::width() const
{
    return mWidth;
}


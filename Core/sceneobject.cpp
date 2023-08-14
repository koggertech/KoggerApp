#include "sceneobject.h"

SceneObject::SceneObject(QString name, QObject *parent)
    : QObject(parent)
    , mName(name)
{}

SceneObject::~SceneObject()
{}

void SceneObject::setName(QString name)
{
    if(mName == name)
        return;

    mName = name;

    Q_EMIT nameChanged(mName);
}

void SceneObject::setVisible(bool isVisible)
{
    if(mIsVisible == isVisible)
        return;

    mIsVisible = isVisible;

    Q_EMIT visibilityChanged(mIsVisible);
}

void SceneObject::setColor(QColor color)
{
    if(mColor == color)
        return;

    mColor = color;

    Q_EMIT colorChanged(color);
}

void SceneObject::setLineWidth(float width)
{
    if(mLineWidth == width)
        return;

    mLineWidth = width;

    Q_EMIT lineWidthChanged(width);
}

QString SceneObject::id() const
{
    return mUuid.toString(QUuid::WithoutBraces);
}

QString SceneObject::name() const
{
    return mName;
}

bool SceneObject::isVisible() const
{
    return mIsVisible;
}

QColor SceneObject::color() const
{
    return mColor;
}

float SceneObject::lineWidth() const
{
    return mLineWidth;
}

SceneObject::SceneObjectType SceneObject::type() const
{
    return SceneObjectType::Unknown;
}

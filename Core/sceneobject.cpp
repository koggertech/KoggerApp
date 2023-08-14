#include "sceneobject.h"

SceneObject::SceneObject(QObject *parent)
    : QObject(parent)
{

}

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

QString SceneObject::id() const
{
    return mUuid.toString(QUuid::WithoutBraces);
}

QString SceneObject::name() const
{
    return mName;
}

void SceneObject::qmlDeclare()
{
    qmlRegisterUncreatableType <SceneObject> ("SceneObject", 1, 0, "SceneObject", "");
    qRegisterMetaType<SceneObject::SceneObjectType>("SceneObjectType");
}

SceneObject::SceneObjectType SceneObject::type() const
{
    return SceneObjectType::Unknown;
}

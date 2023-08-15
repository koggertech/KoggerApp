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

void SceneObject::draw(QOpenGLFunctions* ctx, const QMatrix4x4& mvp,  QMap <QString, QOpenGLShaderProgram*> shaderProgramMap)
{
    Q_UNUSED(ctx)
    Q_UNUSED(mvp)
    Q_UNUSED(shaderProgramMap)
}

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

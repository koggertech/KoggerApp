#include "sceneobjectgroup.h"

SceneObjectGroup::SceneObjectGroup(QObject *parent)
    : SceneObject(parent)
{

}

SceneObject::SceneObjectType SceneObjectGroup::type() const
{
    return SceneObject::SceneObjectType::ObjectsGroup;
}

void SceneObjectGroup::draw(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, QMap<QString, QOpenGLShaderProgram *> shaderProgramMap) const
{
    for(const auto& object : mGroupObjectList)
        object->draw(ctx, mvp, shaderProgramMap);
}

void SceneObjectGroup::addObject(std::shared_ptr<SceneObject> object)
{
    mGroupObjectList.append(object);
}

void SceneObjectGroup::removeObject(std::shared_ptr<SceneObject> object)
{
    auto it = mGroupObjectList.begin();
    while(it != mGroupObjectList.end()){
        if(it->get() == object.get())
            it = mGroupObjectList.erase(it);
        else
            it++;
    }
}

void SceneObjectGroup::removeObject(QString id)
{
    auto it = mGroupObjectList.begin();
    while(it != mGroupObjectList.end()){
        if((*it)->id() == id)
            it = mGroupObjectList.erase(it);
        else
            it++;
    }
}

QObjectList SceneObjectGroup::objects() const
{
    QObjectList list;

    for(const auto& object : mGroupObjectList){
        list.append(
                    reinterpret_cast <QObject*>(object.get())
                );
    }

    return list;
}

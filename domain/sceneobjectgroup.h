#ifndef SCENEOBJECTGROUP_H
#define SCENEOBJECTGROUP_H

#include <sceneobject.h>

class SceneObjectGroup : public SceneObject
{
    Q_OBJECT
public:
    explicit SceneObjectGroup(QObject *parent = nullptr);

    virtual SceneObjectType type() const override;

    virtual void draw(QOpenGLFunctions* ctx,
                      const QMatrix4x4& mvp,
                       QMap <QString, QOpenGLShaderProgram*> shaderProgramMap) const override;

    void addObject(std::shared_ptr <SceneObject> object);

    void removeObject(std::shared_ptr <SceneObject> object);

    void removeObject(QString id);

    QObjectList objects() const;

private:
    QList <std::shared_ptr <SceneObject>> mGroupObjectList;
};

#endif // SCENEOBJECTGROUP_H

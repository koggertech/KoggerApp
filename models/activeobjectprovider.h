#ifndef ACTIVEOBJECTPROVIDER_H
#define ACTIVEOBJECTPROVIDER_H

#include <QObject>
#include <QVariantMap>

#include <memory>

#include <sceneobject.h>

class ActiveObjectProvider : public QObject
{
    Q_OBJECT

    Q_PROPERTY(SceneObject* activeObject READ rawActiveObject NOTIFY activeObjectChanged)

public:

    ActiveObjectProvider(std::shared_ptr <SceneObject> object,
                        QObject *parent = nullptr);

    explicit ActiveObjectProvider(QObject *parent = nullptr);

    void setObject(std::shared_ptr <SceneObject> object);

    std::shared_ptr <SceneObject> activeObject() const;

private:
    Q_INVOKABLE SceneObject* rawActiveObject() const;

Q_SIGNALS:

    void activeObjectChanged(SceneObject* object);

private:

    std::shared_ptr <SceneObject> mp_object;
};

#endif // ACTIVEOBJECTPROVIDER_H

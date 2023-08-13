#ifndef ACTIVEOBJECTPROVIDER_H
#define ACTIVEOBJECTPROVIDER_H

#include <QObject>
#include <QVariantMap>

#include <memory>

#include <vertexobject.h>

class ActiveObjectProvider : public QObject
{
    Q_OBJECT

    Q_PROPERTY(VertexObject* activeObject READ rawActiveObject NOTIFY activeObjectChanged)

public:

    ActiveObjectProvider(std::shared_ptr <VertexObject> object,
                        QObject *parent = nullptr);

    explicit ActiveObjectProvider(QObject *parent = nullptr);

    void setObject(std::shared_ptr <VertexObject> object);

    std::shared_ptr <VertexObject> activeObject() const;

    Q_INVOKABLE VertexObject* rawActiveObject() const;

Q_SIGNALS:

    void activeObjectChanged(VertexObject* object);

private:

    std::shared_ptr <VertexObject> mp_object;
};

#endif // ACTIVEOBJECTPROVIDER_H

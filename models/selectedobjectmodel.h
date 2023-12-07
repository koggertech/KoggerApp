#ifndef SELECTEDOBJECTMODEL_H
#define SELECTEDOBJECTMODEL_H

#include <QObject>
#include <QVariantMap>

#include <memory>

#include <vertexobject.h>

class SelectedObjectModel : public QObject
{
    Q_OBJECT

public:

    Q_INVOKABLE QVariantMap objectParams() const;

public:

    SelectedObjectModel(std::shared_ptr <VertexObject> object,
                        QObject *parent = nullptr);

    explicit SelectedObjectModel(QObject *parent = nullptr);

    void setObject(std::shared_ptr <VertexObject> object);

    std::shared_ptr <VertexObject> object() const;

Q_SIGNALS:

    void objectChanged();

private:

    std::shared_ptr <VertexObject> mpObject;
};

#endif // SELECTEDOBJECTMODEL_H

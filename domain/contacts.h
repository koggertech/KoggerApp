#pragma once

#include "sceneobject.h"


class Contacts : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Contacts)

public:
    /*structures*/
    class ContactsRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        ContactsRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4& model,
                            const QMatrix4x4& view,
                            const QMatrix4x4& projection,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override final;

    private:
        friend class Contacts;
    };

    /*methods*/
    explicit Contacts(QObject* parent = nullptr);
    virtual ~Contacts();

    void clear();


public slots:


signals:

private:

};

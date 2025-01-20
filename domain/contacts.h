#pragma once

#include "sceneobject.h"

#include <QEvent>
#include "plotcash.h"


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

        /*data*/
        QVector<int> indexes_;      // related
        QVector<QVector3D> points_; //
    };

    /*methods*/
    explicit Contacts(QObject* parent = nullptr);
    virtual ~Contacts();

    void clear();
    void setDatasetPtr(Dataset* datasetPtr);

    /*QObject*/
    virtual bool eventFilter(QObject *watched, QEvent *event) override final;

public slots:

signals:

private:
    Dataset* datasetPtr_;

};

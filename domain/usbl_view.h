#pragma once

#include "sceneobject.h"
#include <memory>
#include <QImage>
#include <QOpenGLShaderProgram>
#include <QVector>
#include <QVector3D>
#include <QColor>
#include "plotcash.h"


class UsblView : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(UsblView)

public:
    class UsblViewRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        UsblViewRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx, const QMatrix4x4& mvp,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;
    private:
        friend class UsblView;
        // data
        float pointRadius_;
        bool isTrackVisible_;
    };

    explicit UsblView(QObject* parent = nullptr);
    virtual ~UsblView();

    /*SceneObject*/
    virtual void setData(const QVector<QVector3D>& data, int primitiveType = GL_POINTS) override;
    virtual void clearData() override;
    virtual SceneObjectType type() const override;

    /*UsblView*/
    void setPointRadius(float radius);
    void setTrackVisible(bool state);
    void setDatasetPtr(Dataset* datasetPtr);
    void updateData();

private:
    // data
    Dataset* datasetPtr_;
};

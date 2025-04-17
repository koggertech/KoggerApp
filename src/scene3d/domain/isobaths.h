#pragma once

#include "scene_object.h"
#include "isobaths_processor.h"


class Isobaths : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Isobaths)
    Q_PROPERTY(IsobathsProcessorTask processingTask READ processingTask CONSTANT)

public:
    class IsobathsRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        virtual void render(QOpenGLFunctions* ctx, const QMatrix4x4& mvp, const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

    private:
        friend class Isobaths;
        float verticalScale_ = 1.0f;
    };

    explicit Isobaths(QObject* parent = nullptr);
    virtual ~Isobaths();
    void setProcessingTask(const IsobathsProcessorTask& task);
    virtual void setData(const QVector<QVector3D>& data, int primitiveType = GL_POINTS) override;
    virtual void clearData() override;
    virtual SceneObjectType type() const override;
    IsobathsProcessorTask processingTask() const;

private:
    friend class IsobathsProcessor;
    IsobathsProcessorTask processingTask_;
};

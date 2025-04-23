#pragma once

#include <stdint.h>
#include <QVector>
#include <QVector3D>
#include "scene_object.h"
#include "isobaths_processor.h"


struct ColorInterval
{
    float depth = 0.0f;
    QVector3D color;
    ColorInterval() = default;
    ColorInterval(float d, const QVector3D &c) : depth(d), color(c) {}
};

class Isobaths : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Isobaths)

public:
    class IsobathsRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        IsobathsRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx, const QMatrix4x4 &model, const QMatrix4x4 &view, const QMatrix4x4 &projection, const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

    private:
        friend class Isobaths;

        /*data*/
        QVector<ColorInterval> colorIntervals_;
        float minDepth_;
        float maxDepth_;
        float levelStep_;
        float lineStepSize_;
        GLuint textureId_;

        QVector<QVector3D> lineSegments_;
        QVector3D color_;
        QVector<LabelInfo> labels_;
        float distToFocusPoint_;
    };

    explicit Isobaths(QObject* parent = nullptr);
    virtual ~Isobaths();

    virtual void setData(const QVector<QVector3D>& data, int primitiveType = GL_POINTS) override;
    virtual void clearData() override;
    virtual SceneObjectType type() const override;

    void setProcessingTask(const IsobathsProcessorTask& task);
    IsobathsProcessorTask getProcessingTask() const;

    float getSurfaceStepSize() const;
    void setSurfaceStepSize(float );
    float getLineStepSize() const;
    void setLineStepSize(float val);
    float getLabelStepSize() const;
    void setLabelStepSize(float val);
    void setProcessorResult(const IsobathsProcessorResult& res);

    const QVector<QVector3D>& getRawData() const;
    int  getGridWidth() const;
    int getGridHeight() const;

    QVector<uint8_t>& getTextureTasksRef();
    GLuint getDeinitTextureTask() const;
    GLuint getTextureId() const;
    void setTextureId(GLuint textureId);
    void setCameraDistToFocusPoint(float val);

private:
    friend class IsobathsProcessor;

    /*methods*/
    void rebuildColorIntervals();
    QVector<QVector3D> generateExpandedPalette(int totalColors) const;
    void updateTexture();

    /*data*/
    IsobathsProcessorTask processingTask_;
    float minDepth_;
    float maxDepth_;
    float surfaceStepSize_;
    float lineStepSize_;
    float labelStepSize_;
    GLuint textureId_;
    QVector<uint8_t> textureTask_;
    GLuint toDeleteId_;
};

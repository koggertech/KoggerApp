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
    Q_PROPERTY(IsobathsProcessorTask processingTask READ processingTask CONSTANT) //

public:
    class IsobathsRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        IsobathsRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx, const QMatrix4x4& mvp, const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;


    private:
        friend class Isobaths;

        /*data*/
        QVector<ColorInterval> colorIntervals_;
        float minDepth_;
        float maxDepth_;
        float levelStep_;
        int paletteSize_;
        GLuint textureId_;
    };

    explicit Isobaths(QObject* parent = nullptr);
    virtual ~Isobaths();
    void setProcessingTask(const IsobathsProcessorTask& task);
    virtual void setData(const QVector<QVector3D>& data, int primitiveType = GL_POINTS) override;
    virtual void clearData() override;
    virtual SceneObjectType type() const override;
    IsobathsProcessorTask processingTask() const;

    float getStepSize() const;
    QVector<float> getDepthLevels() const;
    QVector<uint8_t>& getTextureTasksRef();
    GLuint getDeinitTextureTask() const;
    GLuint getTextureId() const;
    void setTextureId(GLuint textureId);
    void setStepSize(float step);

private:
    friend class IsobathsProcessor;

    /*methods*/
    void rebuildColorIntervals();
    QVector<QVector3D> generateExpandedPalette(int totalColors) const;
    void updateTexture();

    /*data*/
    IsobathsProcessorTask processingTask_;
    QVector<ColorInterval> colorIntervals_;
    int paletteSize_;
    float minDepth_;
    float maxDepth_;
    float stepSize_;
    GLuint textureId_;
    QVector<uint8_t> textureTask_;
    GLuint toDeleteId_;
};

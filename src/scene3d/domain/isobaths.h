#pragma once

#include <stdint.h>
#include <QVector>
#include <QVector3D>
#include "isobaths_defs.h"
#include "scene_object.h"

using namespace IsobathUtils;


class Isobaths : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Isobaths)

public:
    class IsobathsRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        IsobathsRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4 &model,
                            const QMatrix4x4 &view,
                            const QMatrix4x4 &projection,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

    private:
        friend class Isobaths;

        QVector<LabelParameters> labels_; // from dataprocessor
        QVector<QVector3D> lineSegments_; // from dataprocessor
        QVector<QVector3D> pts_; // from dataprocessor
        QVector<QVector3D> edgePts_; // from dataprocessor
        QVector3D color_;
        float distToFocusPoint_;
        float minZ_; // from dataprocessor
        float maxZ_; // from dataprocessor
        float levelStep_; // from dataprocessor
        float lineStepSize_; // from dataprocessor
        int colorIntervalsSize_; // from dataprocessor
        GLuint textureId_;
        bool trianglesVisible_;
        bool edgesVisible_;
        bool debugMode_;
    };

    explicit Isobaths(QObject* parent = nullptr);
    virtual ~Isobaths();

    void clear();

    QVector<uint8_t>& getTextureTasksRef();
    GLuint getDeinitTextureTask() const;
    GLuint getTextureId() const;
    void setTextureId(GLuint textureId);
    void setCameraDistToFocusPoint(float val);
    void setDebugMode(bool state);
    void setTrianglesVisible(bool state);
    void setEdgesVisible(bool state);

public slots: // from dataprocessor
    void setLabels(const QVector<IsobathUtils::LabelParameters>& labels);
    void setLineSegments(const QVector<QVector3D>& lineSegments);
    void setPts(const QVector<QVector3D>& pts);
    void setEdgePts(const QVector<QVector3D>& edgePts);
    void setMinZ(float minZ);
    void setMaxZ(float maxZ);
    void setLevelStep(float levelStep);
    void setLineStepSize(float lineStepSize);
    void setTextureTask(const QVector<uint8_t>& textureTask);
    void setColorIntervalsSize(int size);

private:
    QVector<uint8_t> textureTask_;
    GLuint toDeleteId_;
};

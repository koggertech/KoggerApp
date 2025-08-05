#pragma once

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
        float lineStepSize_; // from dataprocessor
    };

    explicit Isobaths(QObject* parent = nullptr);
    virtual ~Isobaths();

    void setCameraDistToFocusPoint(float val);

public slots: // from dataprocessor
    void clear();

    void setLabels(const QVector<IsobathUtils::LabelParameters>& labels);
    void setLineSegments(const QVector<QVector3D>& lineSegments);
    void setPts(const QVector<QVector3D>& pts);
    void setEdgePts(const QVector<QVector3D>& edgePts);
    void setLineStepSize(float lineStepSize);
};

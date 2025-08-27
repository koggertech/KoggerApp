#pragma once

#include <QMap>
#include <QVector>
#include <QVector3D>
#include "isobaths_defs.h"
#include "scene_object.h"


class IsobathsView : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(IsobathsView)

public:
    class IsobathsViewRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        IsobathsViewRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4 &model,
                            const QMatrix4x4 &view,
                            const QMatrix4x4 &projection,
                            const QMap<QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

    private:
        friend class IsobathsView;

        QVector<IsobathUtils::LabelParameters> labels_; // from dataprocessor
        QVector<QVector3D> lineSegments_; // from dataprocessor
        QVector3D color_;
        float distToFocusPoint_;
        float lineStepSize_; // from dataprocessor
        bool mVis_ = false;
    };

    explicit IsobathsView(QObject* parent = nullptr);
    virtual ~IsobathsView();

    void setCameraDistToFocusPoint(float val);
    void setMVisible(bool state) { auto* r = RENDER_IMPL(IsobathsView); r->mVis_ = state; Q_EMIT changed(); };

public slots:
    void clear();
    // from dataprocessor
    void setLabels(const QVector<IsobathUtils::LabelParameters>& labels);
    void setLineSegments(const QVector<QVector3D>& lineSegments);
    void setLineStepSize(float lineStepSize);
};

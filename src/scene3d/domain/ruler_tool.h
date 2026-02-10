#pragma once

#include <QColor>
#include <QVector>
#include <QVector3D>

#include "scene_object.h"

class RulerTool : public SceneObject
{
    Q_OBJECT

public:
    class RulerToolRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        RulerToolRenderImplementation();

        void setEnabled(bool enabled);
        bool isEnabled() const;

        void clear();
        void addPoint(const QVector3D& p);
        void setPreviewPoint(const QVector3D& p);
        void clearPreview();
        void setSelected(bool selected);
        bool isSelected() const;

        int pointsCount() const;

        double totalDistanceXY(bool includePreview) const;
        QVector<QVector3D> polylinePoints(bool includePreview) const;

        void render(QOpenGLFunctions* ctx,
                    const QMatrix4x4& model,
                    const QMatrix4x4& view,
                    const QMatrix4x4& projection,
                    const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>>& shaderProgramMap) const override final;

    private:
        bool enabled_{false};
        QVector<QVector3D> points_;
        bool previewActive_{false};
        QVector3D previewPoint_{};
        bool selected_{false};

        QColor lineColor_{0, 200, 255, 230};
        QColor selectedLineColor_{255, 210, 90, 245};
        float lineWidth_{4.0f};
    };

    explicit RulerTool(QObject* parent = nullptr);
    ~RulerTool() override = default;

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void clear();
    void addPoint(const QVector3D& p);
    void setPreviewPoint(const QVector3D& p);
    void clearPreview();
    void setSelected(bool selected);
    bool isSelected() const;

    int pointsCount() const;
    QVector<QVector3D> polylinePoints(bool includePreview = false) const;

private:
    SceneObjectType type() const override { return SceneObjectType::Unknown; }
};

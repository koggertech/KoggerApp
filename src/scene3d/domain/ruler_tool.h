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
        void setGeometry(const QVector<QVector3D>& vertices,
                         const QVector<QVector<QVector3D>>& lineStrips,
                         const QVector<QVector3D>& midPoints,
                         const QVector<double>& segMeters);
        void setSelected(bool selected);
        bool isSelected() const;

        int vertexCount() const;
        QVector<QVector<QVector3D>> lineStrips() const;

        void render(QOpenGLFunctions* ctx,
                    const QMatrix4x4& model,
                    const QMatrix4x4& view,
                    const QMatrix4x4& projection,
                    const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>>& shaderProgramMap) const final;

    private:
        bool enabled_{false};
        QVector<QVector3D> vertices_;
        QVector<QVector<QVector3D>> lineStrips_;
        QVector<QVector3D> midPoints_;
        QVector<double> segMeters_;
        bool selected_{false};

        QColor lineColor_{45, 64, 89, 255};
        QColor selectedLineColor_{240, 123, 63, 255};
        float lineWidth_{4.0f};
    };

    explicit RulerTool(QObject* parent = nullptr);
    ~RulerTool() override = default;

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void clear();
    void setGeometry(const QVector<QVector3D>& vertices,
                     const QVector<QVector<QVector3D>>& lineStrips,
                     const QVector<QVector3D>& midPoints,
                     const QVector<double>& segMeters);
    void setSelected(bool selected);
    bool isSelected() const;

    int vertexCount() const;
    QVector<QVector<QVector3D>> lineStrips() const;

private:
    SceneObjectType type() const override { return SceneObjectType::Unknown; }
};

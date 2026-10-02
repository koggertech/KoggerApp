#pragma once

#include <cstdint>
#include <memory>
#include <QColor>
#include <QMatrix4x4>
#include <QRect>
#include <QString>
#include <QVector>
#include <QVector2D>
#include <QVector3D>

#include "scene_object.h"

class MissionLayer : public SceneObject
{
    Q_OBJECT

public:
    struct Line
    {
        QVector<QVector3D> points;
        QColor color;
        float widthPx{2.0f};
        bool loop{false};
    };

    struct Fill
    {
        QVector<QVector3D> ring;
        QColor color;
    };

    struct Marker
    {
        enum class Shape : uint8_t
        {
            Circle,
            Ring,
            Plus,
            Cross,
            Move,
            Diamond
        };

        QVector3D world;
        QVector2D offsetPx;
        QColor color;
        float sizePx{8.0f};
        Shape shape{Shape::Circle};
        QString label;
        bool topmost{false};
        bool halo{true};
    };

    struct Arrow
    {
        QVector3D a;
        QVector3D b;
    };

    struct RenderData
    {
        bool enabled{false};
        QVector<Fill> fills;
        QVector<Line> lines;
        QVector<Arrow> arrows;
        QColor arrowColor{255, 255, 255, 220};
        QVector<Marker> markers;
        QColor labelColor{255, 255, 255, 255};
        QColor labelBackground{0, 0, 0, 160};
        QColor haloColor{17, 24, 39, 200};
        QColor lineHaloColor{17, 24, 39, 140};
    };

    class MissionLayerRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        MissionLayerRenderImplementation();
        void setRenderData(RenderData data);

        void render(QOpenGLFunctions* ctx,
                    const QMatrix4x4& model,
                    const QMatrix4x4& view,
                    const QMatrix4x4& projection,
                    const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>>& shaderProgramMap) const override;

        void clearData() override;

    private:
        struct StrokeBatch
        {
            QColor color;
            QVector<QVector3D> verts;
        };

        struct StrokeCache
        {
            bool valid{false};
            float scale{0.0f};
            QMatrix4x4 viewModel;
            QMatrix4x4 projection;
            QRect viewport;
            QVector<QVector3D> halo;
            QVector<StrokeBatch> colors;
        };

        RenderData data_;
        QVector<QVector<QVector3D>> fillTris_;
        std::shared_ptr<StrokeCache> strokeCache_;
    };

    explicit MissionLayer(QObject* parent = nullptr);
    ~MissionLayer() override = default;

    SceneObjectType type() const override;

public slots:
    void setRenderData(MissionLayer::RenderData data);
};

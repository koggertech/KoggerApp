#pragma once

#include <QColor>
#include <QVector>
#include <QVector2D>
#include <QVector3D>

#include "scene_object.h"

class GeoJsonLayer : public SceneObject
{
    Q_OBJECT

public:
    struct Line
    {
        QString id;
        QVector<QVector3D> points;
        QColor color;
        float widthPx{3.0f};
    };

    struct Polygon
    {
        QString id;
        QVector<QVector3D> ring;
        QColor strokeColor;
        float strokeWidthPx{2.0f};
        QColor fillColor;
    };

    struct Marker
    {
        QString featureId;
        int vertexIndex{-1};
        QVector3D world;
        QColor color;
        float sizePx{8.0f};
    };

    struct RenderData
    {
        bool enabled{false};

        QVector<Line> lines;
        QVector<Polygon> polygons;
        QVector<Marker> markers;

        // Draft (draw preview)
        bool draftActive{false};
        bool draftIsPolygon{false};
        QVector<QVector3D> draftPoints;
        bool draftPreviewActive{false};
        QVector3D draftPreviewPoint;
        QColor draftColor{255, 255, 255, 230};
        float draftWidthPx{2.0f};

        // Selection highlight
        bool selectedActive{false};
        QVector3D selectedWorld;
    };

    class GeoJsonLayerRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        GeoJsonLayerRenderImplementation();
        void setRenderData(RenderData data);
        const RenderData& renderData() const;

        void render(QOpenGLFunctions* ctx,
                    const QMatrix4x4& model,
                    const QMatrix4x4& view,
                    const QMatrix4x4& projection,
                    const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>>& shaderProgramMap) const override;

        void clearData() override;

    private:
        static void appendCrossNdc(QVector<QVector2D>& out, const QVector3D& world, float sizePx, const QMatrix4x4& model,
                                  const QMatrix4x4& view, const QMatrix4x4& projection, const QRectF& viewport);

    private:
        RenderData data_;
    };

    explicit GeoJsonLayer(QObject* parent = nullptr);
    ~GeoJsonLayer() override = default;

    SceneObjectType type() const override;

public slots:
    void setRenderData(RenderData data);
    void clear();
};


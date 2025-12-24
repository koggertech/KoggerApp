#include "geojson_layer.h"

#include <QHash>

#include "draw_utils.h"

GeoJsonLayer::GeoJsonLayerRenderImplementation::GeoJsonLayerRenderImplementation()
{
    m_isVisible = true;
}

void GeoJsonLayer::GeoJsonLayerRenderImplementation::setRenderData(RenderData data)
{
    data_ = std::move(data);
}

const GeoJsonLayer::RenderData& GeoJsonLayer::GeoJsonLayerRenderImplementation::renderData() const
{
    return data_;
}

void GeoJsonLayer::GeoJsonLayerRenderImplementation::clearData()
{
    data_ = RenderData{};
}

void GeoJsonLayer::GeoJsonLayerRenderImplementation::appendCrossNdc(
    QVector<QVector2D>& out,
    const QVector3D& world,
    float sizePx,
    const QMatrix4x4& model,
    const QMatrix4x4& view,
    const QMatrix4x4& projection,
    const QRectF& viewport)
{
    const float halfW = viewport.width() * 0.5f;
    const float halfH = viewport.height() * 0.5f;
    if (halfW <= 0.0f || halfH <= 0.0f) {
        return;
    }

    const QVector3D win = world.project(view * model, projection, viewport.toRect());
    const float nx = (win.x() / halfW) - 1.0f;
    const float ny = (win.y() / halfH) - 1.0f;

    const float dx = sizePx / halfW;
    const float dy = sizePx / halfH;

    out.push_back({nx - dx, ny});
    out.push_back({nx + dx, ny});
    out.push_back({nx, ny - dy});
    out.push_back({nx, ny + dy});
}

void GeoJsonLayer::GeoJsonLayerRenderImplementation::render(
    QOpenGLFunctions* ctx,
    const QMatrix4x4& model,
    const QMatrix4x4& view,
    const QMatrix4x4& projection,
    const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>>& shaderProgramMap) const
{
    // m_isVisible, data_.enabled
    // qDebug() << "GeoJsonLayer::GeoJsonLayerRenderImplementation::render called.";
    // qDebug() << "m_isVisible:" << m_isVisible << ", data_.enabled:" << data_.enabled;
    if (!m_isVisible || !data_.enabled) {
        return;
    }

    const QRectF viewport = DrawUtils::viewportRect(ctx);

    // World-space geometry (lines + polygons)
    if (auto it = shaderProgramMap.find("static"); it != shaderProgramMap.end() && it.value()) {
        auto sp = it.value();
        if (sp->bind()) {
            const int posLoc = sp->attributeLocation("position");
            const int colorLoc = sp->uniformLocation("color");
            const int matrixLoc = sp->uniformLocation("matrix");

            sp->setUniformValue(matrixLoc, projection * view * model);
            sp->enableAttributeArray(posLoc);

            // Polygons (fill + stroke)
            for (const auto& poly : data_.polygons) {
                const auto& ring = poly.ring;
                if (ring.size() >= 3) {
                    // Fill (triangle fan)
                    sp->setUniformValue(colorLoc, DrawUtils::colorToVector4d(poly.fillColor));
                    for (int i = 1; i < ring.size() - 1; ++i) {
                        QVector<QVector3D> tri;
                        tri.reserve(3);
                        tri.push_back(ring[0]);
                        tri.push_back(ring[i]);
                        tri.push_back(ring[i + 1]);
                        sp->setAttributeArray(posLoc, tri.constData());
                        ctx->glDrawArrays(GL_TRIANGLES, 0, tri.size());
                    }

                    // Stroke
                    if (ring.size() >= 2) {
                        sp->setUniformValue(colorLoc, DrawUtils::colorToVector4d(poly.strokeColor));
                        sp->setAttributeArray(posLoc, ring.constData());
                        ctx->glLineWidth(poly.strokeWidthPx);
                        ctx->glDrawArrays(GL_LINE_LOOP, 0, ring.size());
                        ctx->glLineWidth(1.0f);
                    }
                }
            }

            // Lines
            for (const auto& line : data_.lines) {
                if (line.points.size() < 2) {
                    continue;
                }

                qDebug() << "Drawing line" << line.id << "with" << line.points.size() << "points.";
                qDebug() << "Points:" << line.points;
                sp->setUniformValue(colorLoc, DrawUtils::colorToVector4d(line.color));
                sp->setAttributeArray(posLoc, line.points.constData());
                ctx->glLineWidth(line.widthPx);
                ctx->glDrawArrays(GL_LINE_STRIP, 0, line.points.size());
                ctx->glLineWidth(1.0f);
            }

            // Draft polyline
            if (data_.draftActive) {
                QVector<QVector3D> pts = data_.draftPoints;
                if (data_.draftPreviewActive) {
                    pts.push_back(data_.draftPreviewPoint);
                }

                if (pts.size() >= 2) {
                    sp->setUniformValue(colorLoc, DrawUtils::colorToVector4d(data_.draftColor));
                    sp->setAttributeArray(posLoc, pts.constData());
                    ctx->glLineWidth(data_.draftWidthPx);
                    ctx->glDrawArrays(GL_LINE_STRIP, 0, pts.size());
                    ctx->glLineWidth(1.0f);

                    if (data_.draftIsPolygon && pts.size() >= 3) {
                        QVector<QVector3D> closing = {pts.back(), pts.front()};
                        sp->setAttributeArray(posLoc, closing.constData());
                        ctx->glDrawArrays(GL_LINES, 0, closing.size());
                    }
                }
            }

            sp->disableAttributeArray(posLoc);
            sp->release();
        }
    }

    // Screen-space markers
    if (auto it2 = shaderProgramMap.find("static_sec"); it2 != shaderProgramMap.end() && it2.value()) {
        QHash<QRgb, QVector<QVector2D>> byColor;
        byColor.reserve(data_.markers.size() + 2);

        for (const auto& m : data_.markers) {
            auto& vec = byColor[m.color.rgba()];
            vec.reserve(vec.size() + 4);
            appendCrossNdc(vec, m.world, m.sizePx, model, view, projection, viewport);
        }

        if (data_.selectedActive) {
            auto& vec = byColor[QColor(255, 215, 0, 230).rgba()];
            vec.reserve(vec.size() + 4);
            appendCrossNdc(vec, data_.selectedWorld, 14.0f, model, view, projection, viewport);
        }

        auto sp = it2.value();
        for (auto it = byColor.constBegin(); it != byColor.constEnd(); ++it) {
            const auto& markersNdc = it.value();
            if (markersNdc.isEmpty()) {
                continue;
            }
            if (!sp->bind()) {
                break;
            }

            const int colorLoc = sp->uniformLocation("color");
            sp->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor::fromRgba(it.key())));
            sp->enableAttributeArray(0);
            sp->setAttributeArray(0, markersNdc.constData());
            ctx->glLineWidth(2.0f);
            ctx->glDrawArrays(GL_LINES, 0, markersNdc.size());
            ctx->glLineWidth(1.0f);
            sp->disableAttributeArray(0);
            sp->release();
        }
    }
}

GeoJsonLayer::GeoJsonLayer(QObject* parent)
    : SceneObject(new GeoJsonLayerRenderImplementation(), parent, QStringLiteral("GeoJsonLayer"))
{
    setVisible(true);
}

SceneObject::SceneObjectType GeoJsonLayer::type() const
{
    return SceneObject::SceneObjectType::Unknown;
}

void GeoJsonLayer::setRenderData(RenderData data)
{
    auto* r = dynamic_cast<GeoJsonLayerRenderImplementation*>(m_renderImpl);
    if (!r) {
        return;
    }
    r->setRenderData(std::move(data));
    emit changed();
}

void GeoJsonLayer::clear()
{
    auto* r = dynamic_cast<GeoJsonLayerRenderImplementation*>(m_renderImpl);
    if (!r) {
        return;
    }
    r->clearData();
    emit changed();
}

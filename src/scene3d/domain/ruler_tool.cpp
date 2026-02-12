#include "ruler_tool.h"

#include <cmath>
#include <QString>

#include "draw_utils.h"
#include "text_renderer.h"
#include "themes.h"

extern Themes theme;

static QString formatDistanceMeters(double meters)
{
    if (!std::isfinite(meters)) {
        return QStringLiteral("—");
    }

    if (meters >= 1000.0) {
        return QString("%1 km").arg(meters / 1000.0, 0, 'f', 3);
    }

    return QString("%1 m").arg(meters, 0, 'f', 2);
}

RulerTool::RulerToolRenderImplementation::RulerToolRenderImplementation()
{
    m_isVisible = true;
}

void RulerTool::RulerToolRenderImplementation::setEnabled(bool enabled)
{
    enabled_ = enabled;
}

bool RulerTool::RulerToolRenderImplementation::isEnabled() const
{
    return enabled_;
}

void RulerTool::RulerToolRenderImplementation::clear()
{
    points_.clear();
    previewActive_ = false;
    selected_ = false;
}

void RulerTool::RulerToolRenderImplementation::addPoint(const QVector3D& p)
{
    points_.push_back(p);
}

void RulerTool::RulerToolRenderImplementation::setPreviewPoint(const QVector3D& p)
{
    previewPoint_ = p;
    previewActive_ = true;
}

void RulerTool::RulerToolRenderImplementation::clearPreview()
{
    previewActive_ = false;
}

void RulerTool::RulerToolRenderImplementation::setSelected(bool selected)
{
    selected_ = selected;
}

bool RulerTool::RulerToolRenderImplementation::isSelected() const
{
    return selected_;
}

int RulerTool::RulerToolRenderImplementation::pointsCount() const
{
    return points_.size();
}

double RulerTool::RulerToolRenderImplementation::totalDistanceXY(bool includePreview) const
{
    const auto pts = polylinePoints(includePreview);
    if (pts.size() < 2) {
        return 0.0;
    }

    double sum = 0.0;
    for (int i = 1; i < pts.size(); ++i) {
        const double dx = static_cast<double>(pts[i].x() - pts[i - 1].x());
        const double dy = static_cast<double>(pts[i].y() - pts[i - 1].y());
        sum += std::sqrt(dx * dx + dy * dy);
    }
    return sum;
}

QVector<QVector3D> RulerTool::RulerToolRenderImplementation::polylinePoints(bool includePreview) const
{
    QVector<QVector3D> out = points_;
    if (includePreview && previewActive_) {
        out.push_back(previewPoint_);
    }
    return out;
}

void RulerTool::RulerToolRenderImplementation::render(
    QOpenGLFunctions* ctx,
    const QMatrix4x4& model,
    const QMatrix4x4& view,
    const QMatrix4x4& projection,
    const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>>& shaderProgramMap) const
{
    if (!enabled_) {
        return;
    }

    const auto pts = polylinePoints(/*includePreview*/ true);
    if (pts.size() < 2) {
        return;
    }

    auto shaderIt = shaderProgramMap.find("static");
    if (shaderIt == shaderProgramMap.end() || !shaderIt.value()) {
        return;
    }

    auto shaderProgram = shaderIt.value();
    if (!shaderProgram->bind()) {
        return;
    }

    const int posLoc = shaderProgram->attributeLocation("position");
    const int colorLoc = shaderProgram->uniformLocation("color");
    const int matrixLoc = shaderProgram->uniformLocation("matrix");

    const QColor drawColor = selected_ ? selectedLineColor_ : lineColor_;
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(drawColor));
    shaderProgram->setUniformValue(matrixLoc, projection * view * model);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, pts.constData());

    ctx->glLineWidth(lineWidth_);
    ctx->glDrawArrays(GL_LINE_STRIP, 0, pts.size());
    ctx->glLineWidth(1.0f);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();

    const QRectF viewport = DrawUtils::viewportRect(ctx);
    // Markers (vertices + midpoints) in screen space so they stay the same size on screen
    {
        auto shaderIt2 = shaderProgramMap.find("static_sec");
        if (shaderIt2 != shaderProgramMap.end() && shaderIt2.value()) {
            const float halfW = viewport.width() * 0.5f;
            const float halfH = viewport.height() * 0.5f;
            if (halfW > 0.0f && halfH > 0.0f) {
                QVector<QVector2D> markersNdc;
                markersNdc.reserve((pts.size() * 4) + ((pts.size() - 1) * 4));

                const float vertexPx = 10.0f;
                const float midPx = 7.0f;
                const float dxV = vertexPx / halfW;
                const float dyV = vertexPx / halfH;
                const float dxM = midPx / halfW;
                const float dyM = midPx / halfH;

                auto addCrossNdc = [&](const QVector3D& world, float dx, float dy) {
                    const QVector3D win = world.project(view * model, projection, viewport.toRect());
                    const float nx = (win.x() / halfW) - 1.0f;
                    const float ny = (win.y() / halfH) - 1.0f;

                    markersNdc.push_back({nx - dx, ny});
                    markersNdc.push_back({nx + dx, ny});
                    markersNdc.push_back({nx, ny - dy});
                    markersNdc.push_back({nx, ny + dy});
                };

                for (const auto& p : pts) {
                    addCrossNdc(p, dxV, dyV);
                }
                for (int i = 1; i < pts.size(); ++i) {
                    addCrossNdc((pts[i - 1] + pts[i]) * 0.5f, dxM, dyM);
                }

                if (!markersNdc.isEmpty()) {
                    auto sp = shaderIt2.value();
                    if (sp->bind()) {
                        const int colorLoc2 = sp->uniformLocation("color");
                        sp->setUniformValue(colorLoc2, DrawUtils::colorToVector4d(QColor(234, 84, 85, 255)));
                        sp->enableAttributeArray(0);
                        sp->setAttributeArray(0, markersNdc.constData());

                        ctx->glLineWidth(lineWidth_);
                        ctx->glDrawArrays(GL_LINES, 0, markersNdc.size());
                        ctx->glLineWidth(1.0f);

                        sp->disableAttributeArray(0);
                        sp->release();
                    }
                }
            }
        }
    }

    QMatrix4x4 textProjection;
    textProjection.ortho(viewport.toRect());

    TextRenderer::instance().setColor(theme.textSolidColor());
    TextRenderer::instance().setBackgroundColor(QColor(0, 0, 0, 160));

    // Segment labels (XY length)
    for (int i = 1; i < pts.size(); ++i) {
        const double dx = static_cast<double>(pts[i].x() - pts[i - 1].x());
        const double dy = static_cast<double>(pts[i].y() - pts[i - 1].y());
        const double segMeters = std::sqrt(dx * dx + dy * dy);

        const QVector3D mid = (pts[i - 1] + pts[i]) * 0.5f;
        QVector2D midScreen = mid.project(view * model, projection, viewport.toRect()).toVector2D();
        midScreen.setY(viewport.height() - midScreen.y());

        midScreen += QVector2D(0.0f, (i % 2 == 0) ? -18.0f : -34.0f);

        TextRenderer::instance().render(formatDistanceMeters(segMeters), 1.0f, midScreen, true, ctx, textProjection, shaderProgramMap);
    }

    // Total label (XY only)
    const double totalMeters = totalDistanceXY(/*includePreview*/ true);
    const QString label = QStringLiteral("Σ %1").arg(formatDistanceMeters(totalMeters));

    QVector3D anchor = pts.back();
    QVector2D screen = anchor.project(view * model, projection, viewport.toRect()).toVector2D();
    screen.setY(viewport.height() - screen.y());
    screen += QVector2D(12.0f, -14.0f);

    TextRenderer::instance().render(label, 1.5f, screen, true, ctx, textProjection, shaderProgramMap);
}

RulerTool::RulerTool(QObject* parent)
    : SceneObject(new RulerToolRenderImplementation(), parent, QStringLiteral("RulerTool"))
{
    setVisible(true);
}

void RulerTool::setEnabled(bool enabled)
{
    auto* r = RENDER_IMPL(RulerTool);
    r->setEnabled(enabled);
    Q_EMIT changed();
}

bool RulerTool::isEnabled() const
{
    return RENDER_IMPL(RulerTool)->isEnabled();
}

void RulerTool::clear()
{
    auto* r = RENDER_IMPL(RulerTool);
    r->clear();
    Q_EMIT changed();
}

void RulerTool::addPoint(const QVector3D& p)
{
    auto* r = RENDER_IMPL(RulerTool);
    r->addPoint(p);
    Q_EMIT changed();
}

void RulerTool::setPreviewPoint(const QVector3D& p)
{
    auto* r = RENDER_IMPL(RulerTool);
    r->setPreviewPoint(p);
    Q_EMIT changed();
}

void RulerTool::clearPreview()
{
    auto* r = RENDER_IMPL(RulerTool);
    r->clearPreview();
    Q_EMIT changed();
}

void RulerTool::setSelected(bool selected)
{
    auto* r = RENDER_IMPL(RulerTool);
    r->setSelected(selected);
    Q_EMIT changed();
}

bool RulerTool::isSelected() const
{
    return RENDER_IMPL(RulerTool)->isSelected();
}

int RulerTool::pointsCount() const
{
    return RENDER_IMPL(RulerTool)->pointsCount();
}

QVector<QVector3D> RulerTool::polylinePoints(bool includePreview) const
{
    return RENDER_IMPL(RulerTool)->polylinePoints(includePreview);
}

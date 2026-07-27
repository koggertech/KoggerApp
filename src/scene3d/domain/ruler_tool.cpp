#include "ruler_tool.h"

#include <cmath>
#include <QCoreApplication>
#include <QString>

#include "draw_utils.h"
#include "text_renderer.h"
#include "themes.h"

static QString formatDistanceMeters(double meters)
{
    if (!std::isfinite(meters)) {
        return QStringLiteral("—");
    }

    if (meters >= 1000.0) {
        return QCoreApplication::translate("RulerTool", "%1 km").arg(meters / 1000.0, 0, 'f', 3);
    }

    return QCoreApplication::translate("RulerTool", "%1 m").arg(meters, 0, 'f', 2);
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
    vertices_.clear();
    lineStrips_.clear();
    midPoints_.clear();
    segMeters_.clear();
}

void RulerTool::RulerToolRenderImplementation::setGeometry(const QVector<QVector3D>& vertices,
                                                           const QVector<QVector<QVector3D>>& lineStrips,
                                                           const QVector<QVector3D>& midPoints,
                                                           const QVector<double>& segMeters)
{
    vertices_ = vertices;
    lineStrips_ = lineStrips;
    midPoints_ = midPoints;
    segMeters_ = segMeters;
}

void RulerTool::RulerToolRenderImplementation::setSelected(bool selected)
{
    selected_ = selected;
}

bool RulerTool::RulerToolRenderImplementation::isSelected() const
{
    return selected_;
}

int RulerTool::RulerToolRenderImplementation::vertexCount() const
{
    return vertices_.size();
}

QVector<QVector<QVector3D>> RulerTool::RulerToolRenderImplementation::lineStrips() const
{
    return lineStrips_;
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

    bool hasLine = false;
    for (const auto& strip : lineStrips_) {
        if (strip.size() >= 2) {
            hasLine = true;
            break;
        }
    }
    if (!hasLine) {
        return;
    }

    auto shaderIt = shaderProgramMap.find("static");
    if (shaderIt == shaderProgramMap.end() || !shaderIt.value()) {
        return;
    }

    const auto& shaderProgram = shaderIt.value();
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

    ctx->glLineWidth(lineWidth_ * static_cast<float>(renderScale()));
    for (const auto& strip : lineStrips_) {
        if (strip.size() < 2) {
            continue;
        }
        shaderProgram->setAttributeArray(posLoc, strip.constData());
        ctx->glDrawArrays(GL_LINE_STRIP, 0, strip.size());
    }
    ctx->glLineWidth(1.0f);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();

    const QRectF viewport = DrawUtils::viewportRect(ctx);
    // Markers (vertices + segment midpoints) in screen space so they stay the same size on screen
    {
        auto shaderIt2 = shaderProgramMap.find("static_sec");
        if (shaderIt2 != shaderProgramMap.end() && shaderIt2.value()) {
            const float halfW = viewport.width() * 0.5f;
            const float halfH = viewport.height() * 0.5f;
            if (halfW > 0.0f && halfH > 0.0f) {
                QVector<QVector2D> markersNdc;
                markersNdc.reserve((vertices_.size() * 4) + (midPoints_.size() * 4));

                const float uiScale = static_cast<float>(renderScale());
                const float vertexPx = 10.0f * uiScale;
                const float midPx = 7.0f * uiScale;
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

                for (const auto& p : vertices_) {
                    addCrossNdc(p, dxV, dyV);
                }
                for (const auto& p : midPoints_) {
                    addCrossNdc(p, dxM, dyM);
                }

                if (!markersNdc.isEmpty()) {
                    const auto& sp = shaderIt2.value();
                    if (sp->bind()) {
                        const int colorLoc2 = sp->uniformLocation("color");
                        sp->setUniformValue(colorLoc2, DrawUtils::colorToVector4d(QColor(234, 84, 85, 255)));
                        sp->enableAttributeArray(0);
                        sp->setAttributeArray(0, markersNdc.constData());

                        ctx->glLineWidth(lineWidth_ * static_cast<float>(renderScale()));
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

    TextRenderer::instance().setColor(QColor(255, 255, 255));
    TextRenderer::instance().setBackgroundColor(QColor(0, 0, 0, 160));
    QVector<TextRenderer::Text2DItem> labelItems;
    labelItems.reserve(midPoints_.size() + 1);

    for (int i = 0; i < midPoints_.size(); ++i) {
        const double segMeters = (i < segMeters_.size()) ? segMeters_[i] : 0.0;

        QVector2D midScreen = midPoints_[i].project(view * model, projection, viewport.toRect()).toVector2D();
        midScreen.setY(viewport.height() - midScreen.y());
        midScreen += QVector2D(0.0f, (((i + 1) % 2 == 0) ? -18.0f : -34.0f) * static_cast<float>(renderScale()));

        labelItems.append(TextRenderer::Text2DItem{formatDistanceMeters(segMeters), 1.0f, midScreen, true});
    }

    double totalMeters = 0.0;
    for (const double m : segMeters_) {
        totalMeters += m;
    }
    const QString label = QCoreApplication::translate("RulerTool", "Σ %1").arg(formatDistanceMeters(totalMeters));

    if (vertices_.isEmpty()) {
        TextRenderer::instance().render2DBatch(labelItems, ctx, textProjection, shaderProgramMap);
        return;
    }
    const QVector3D anchor = vertices_.back();
    QVector2D screen = anchor.project(view * model, projection, viewport.toRect()).toVector2D();
    screen.setY(viewport.height() - screen.y());
    screen += QVector2D(12.0f * static_cast<float>(renderScale()), -14.0f * static_cast<float>(renderScale()));

    labelItems.append(TextRenderer::Text2DItem{label, 1.5f, screen, true});

    TextRenderer::instance().render2DBatch(labelItems, ctx, textProjection, shaderProgramMap);
}

RulerTool::RulerTool(QObject* parent)
    : SceneObject(new RulerToolRenderImplementation(), parent, QStringLiteral("RulerTool"))
{
    setVisible(true);
}

void RulerTool::setEnabled(bool enabled)
{
    RENDER_IMPL(RulerTool)->setEnabled(enabled);
    Q_EMIT changed();
}

bool RulerTool::isEnabled() const
{
    return RENDER_IMPL(RulerTool)->isEnabled();
}

void RulerTool::clear()
{
    RENDER_IMPL(RulerTool)->clear();
    Q_EMIT changed();
}

void RulerTool::setGeometry(const QVector<QVector3D>& vertices,
                            const QVector<QVector<QVector3D>>& lineStrips,
                            const QVector<QVector3D>& midPoints,
                            const QVector<double>& segMeters)
{
    RENDER_IMPL(RulerTool)->setGeometry(vertices, lineStrips, midPoints, segMeters);
    Q_EMIT changed();
}

void RulerTool::setSelected(bool selected)
{
    RENDER_IMPL(RulerTool)->setSelected(selected);
    Q_EMIT changed();
}

bool RulerTool::isSelected() const
{
    return RENDER_IMPL(RulerTool)->isSelected();
}

int RulerTool::vertexCount() const
{
    return RENDER_IMPL(RulerTool)->vertexCount();
}

QVector<QVector<QVector3D>> RulerTool::lineStrips() const
{
    return RENDER_IMPL(RulerTool)->lineStrips();
}

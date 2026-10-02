#include "mission_layer.h"

#include <algorithm>
#include <cmath>
#include <map>

#include "draw_utils.h"
#include "text_renderer.h"
#include "themes.h"

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kHaloZ = -0.98f;
constexpr float kLineZ = -1.0f;
constexpr float kHaloWidthPx = 2.0f;
constexpr float kHaloExtendPx = 1.0f;
constexpr float kArrowLenPx = 7.0f;
constexpr float kArrowMinSegmentFactor = 2.0f;
constexpr float kMarkerLineWidthPx = 2.0f;
constexpr float kLabelOffsetPx = 4.0f;
constexpr float kCos30 = 0.866f;
constexpr float kSin30 = 0.5f;
constexpr float kSqrtHalf = 0.7071f;
constexpr float kMoveHeadFraction = 0.4f;
constexpr float kSegmentsPerRadiusPx = 4.0f;
constexpr int kMinCircleSegments = 16;
constexpr int kMaxCircleSegments = 72;
constexpr int kMinJoinSegments = 8;
constexpr int kMaxJoinSegments = 24;
constexpr float kJoinAngleCos = 0.995f;

struct ScreenSpace
{
    float halfW = 0.0f;
    float halfH = 0.0f;
    QMatrix4x4 viewModel;
    QMatrix4x4 projection;
    QRect viewport;

    bool valid() const { return halfW > 0.0f && halfH > 0.0f; }

    QVector2D ndc(const QVector3D& world) const
    {
        const QVector3D win = world.project(viewModel, projection, viewport);
        return QVector2D((win.x() / halfW) - 1.0f, (win.y() / halfH) - 1.0f);
    }

    QVector2D window(const QVector3D& world) const
    {
        const QVector3D win = world.project(viewModel, projection, viewport);
        return QVector2D(win.x(), win.y());
    }

    QVector2D toNdc(const QVector2D& w) const
    {
        return QVector2D((w.x() / halfW) - 1.0f, (w.y() / halfH) - 1.0f);
    }
};

const QVector<QVector2D>& unitCircle(int segments)
{
    static std::map<int, QVector<QVector2D>> cache;
    auto it = cache.find(segments);
    if (it == cache.end()) {
        QVector<QVector2D> pts;
        pts.reserve(segments + 1);
        for (int i = 0; i <= segments; ++i) {
            const float a = float(i) * (kPi * 2.0f) / float(segments);
            pts.append(QVector2D(std::cos(a), std::sin(a)));
        }
        it = cache.emplace(segments, std::move(pts)).first;
    }
    return it->second;
}

int circleSegments(float radiusPx)
{
    return std::clamp(static_cast<int>(radiusPx * kSegmentsPerRadiusPx), kMinCircleSegments, kMaxCircleSegments);
}

int joinSegments(float radiusPx)
{
    return std::clamp(static_cast<int>(radiusPx * kSegmentsPerRadiusPx), kMinJoinSegments, kMaxJoinSegments);
}

void appendCircle(QVector<QVector2D>& out, const QVector2D& c, float rx, float ry, int segments)
{
    const auto& u = unitCircle(segments);
    for (int i = 0; i < segments; ++i) {
        out.push_back(c);
        out.push_back({c.x() + u[i].x() * rx, c.y() + u[i].y() * ry});
        out.push_back({c.x() + u[i + 1].x() * rx, c.y() + u[i + 1].y() * ry});
    }
}

void appendRing(QVector<QVector2D>& out, const QVector2D& c, float rx, float ry, int segments)
{
    const auto& u = unitCircle(segments);
    for (int i = 0; i < segments; ++i) {
        out.push_back({c.x() + u[i].x() * rx, c.y() + u[i].y() * ry});
        out.push_back({c.x() + u[i + 1].x() * rx, c.y() + u[i + 1].y() * ry});
    }
}

void appendPlus(QVector<QVector2D>& out, const QVector2D& c, float dx, float dy)
{
    out.push_back({c.x() - dx, c.y()});
    out.push_back({c.x() + dx, c.y()});
    out.push_back({c.x(), c.y() - dy});
    out.push_back({c.x(), c.y() + dy});
}

void appendCross(QVector<QVector2D>& out, const QVector2D& c, float dx, float dy)
{
    const float kx = dx * kSqrtHalf;
    const float ky = dy * kSqrtHalf;
    out.push_back({c.x() - kx, c.y() - ky});
    out.push_back({c.x() + kx, c.y() + ky});
    out.push_back({c.x() - kx, c.y() + ky});
    out.push_back({c.x() + kx, c.y() - ky});
}

void appendMove(QVector<QVector2D>& out, const QVector2D& c, float dx, float dy)
{
    appendPlus(out, c, dx, dy);
    const float hx = dx * kMoveHeadFraction;
    const float hy = dy * kMoveHeadFraction;
    out.push_back({c.x() + dx, c.y()}); out.push_back({c.x() + dx - hx, c.y() + hy});
    out.push_back({c.x() + dx, c.y()}); out.push_back({c.x() + dx - hx, c.y() - hy});
    out.push_back({c.x() - dx, c.y()}); out.push_back({c.x() - dx + hx, c.y() + hy});
    out.push_back({c.x() - dx, c.y()}); out.push_back({c.x() - dx + hx, c.y() - hy});
    out.push_back({c.x(), c.y() + dy}); out.push_back({c.x() + hx, c.y() + dy - hy});
    out.push_back({c.x(), c.y() + dy}); out.push_back({c.x() - hx, c.y() + dy - hy});
    out.push_back({c.x(), c.y() - dy}); out.push_back({c.x() + hx, c.y() - dy + hy});
    out.push_back({c.x(), c.y() - dy}); out.push_back({c.x() - hx, c.y() - dy + hy});
}

void appendDiamond(QVector<QVector2D>& out, const QVector2D& c, float dx, float dy)
{
    const QVector2D top(c.x(), c.y() + dy);
    const QVector2D bottom(c.x(), c.y() - dy);
    const QVector2D left(c.x() - dx, c.y());
    const QVector2D right(c.x() + dx, c.y());
    out.push_back(top); out.push_back(left); out.push_back(bottom);
    out.push_back(top); out.push_back(bottom); out.push_back(right);
}

void appendStroke(QVector<QVector3D>& out, const QVector<QVector2D>& px, bool loop, float widthPx, float z, const ScreenSpace& ss)
{
    const float hw = widthPx * 0.5f;
    const int n = px.size();
    auto ndc = [&](const QVector2D& w) { return QVector3D(w.x() / ss.halfW - 1.0f, w.y() / ss.halfH - 1.0f, z); };

    const int segCount = loop ? n : n - 1;
    for (int i = 0; i < segCount; ++i) {
        const QVector2D a = px[i];
        const QVector2D b = px[(i + 1) % n];
        QVector2D d = b - a;
        const float len = d.length();
        if (len < 1e-3f) {
            continue;
        }
        d /= len;
        const QVector2D nrm(-d.y() * hw, d.x() * hw);
        const QVector3D p0 = ndc(a + nrm);
        const QVector3D p1 = ndc(b + nrm);
        const QVector3D p2 = ndc(b - nrm);
        const QVector3D p3 = ndc(a - nrm);
        out.push_back(p0); out.push_back(p1); out.push_back(p2);
        out.push_back(p0); out.push_back(p2); out.push_back(p3);
    }

    const int segments = joinSegments(hw);
    const auto& u = unitCircle(segments);
    const float rx = hw / ss.halfW;
    const float ry = hw / ss.halfH;
    for (int i = 0; i < n; ++i) {
        const bool cap = !loop && (i == 0 || i == n - 1);
        if (!cap) {
            QVector2D d0 = px[i] - px[(i - 1 + n) % n];
            QVector2D d1 = px[(i + 1) % n] - px[i];
            if (d0.length() < 1e-3f || d1.length() < 1e-3f) {
                continue;
            }
            d0.normalize();
            d1.normalize();
            if (QVector2D::dotProduct(d0, d1) > kJoinAngleCos) {
                continue;
            }
        }
        const QVector3D c = ndc(px[i]);
        for (int s = 0; s < segments; ++s) {
            out.push_back(c);
            out.push_back(QVector3D(c.x() + u[s].x() * rx, c.y() + u[s].y() * ry, z));
            out.push_back(QVector3D(c.x() + u[s + 1].x() * rx, c.y() + u[s + 1].y() * ry, z));
        }
    }
}

QVector<QVector2D> extendSegments(const QVector<QVector2D>& verts, const ScreenSpace& ss)
{
    QVector<QVector2D> out;
    out.reserve(verts.size());
    for (int i = 0; i + 1 < verts.size(); i += 2) {
        const QVector2D p0(verts[i].x() * ss.halfW, verts[i].y() * ss.halfH);
        const QVector2D p1(verts[i + 1].x() * ss.halfW, verts[i + 1].y() * ss.halfH);
        QVector2D d = p1 - p0;
        const float len = d.length();
        d = len > 1e-4f ? d * (kHaloExtendPx / len) : QVector2D(0.0f, 0.0f);
        const QVector2D e0 = p0 - d;
        const QVector2D e1 = p1 + d;
        out.append(QVector2D(e0.x() / ss.halfW, e0.y() / ss.halfH));
        out.append(QVector2D(e1.x() / ss.halfW, e1.y() / ss.halfH));
    }
    return out;
}

void drawTriangles3(QOpenGLFunctions* ctx, QOpenGLShaderProgram& sp, int colorLoc, const QVector<QVector3D>& verts, const QColor& color)
{
    if (verts.isEmpty()) {
        return;
    }
    sp.setUniformValue(colorLoc, DrawUtils::colorToVector4d(color));
    sp.setAttributeArray(0, verts.constData());
    ctx->glDrawArrays(GL_TRIANGLES, 0, verts.size());
}

void draw2d(QOpenGLFunctions* ctx, QOpenGLShaderProgram& sp, int colorLoc, const QVector<QVector2D>& verts, const QColor& color, GLenum mode, float lineWidth)
{
    if (verts.isEmpty()) {
        return;
    }
    sp.setUniformValue(colorLoc, DrawUtils::colorToVector4d(color));
    sp.setAttributeArray(0, verts.constData());
    if (mode == GL_LINES) {
        ctx->glLineWidth(lineWidth);
    }
    ctx->glDrawArrays(mode, 0, verts.size());
    if (mode == GL_LINES) {
        ctx->glLineWidth(1.0f);
    }
}

struct MarkerBatch
{
    QRgb color;
    bool halo;
    QVector<QVector2D> verts;
};

QVector<QVector2D>& batchFor(QVector<MarkerBatch>& batches, QRgb color, bool halo)
{
    if (batches.isEmpty() || batches.last().color != color || batches.last().halo != halo) {
        batches.append(MarkerBatch{color, halo, {}});
    }
    return batches.last().verts;
}

} // namespace

MissionLayer::MissionLayerRenderImplementation::MissionLayerRenderImplementation()
    : strokeCache_(std::make_shared<StrokeCache>())
{
    m_isVisible = true;
}

void MissionLayer::MissionLayerRenderImplementation::setRenderData(RenderData data)
{
    data_ = std::move(data);
    fillTris_.clear();
    fillTris_.reserve(data_.fills.size());
    for (const auto& fill : data_.fills) {
        fillTris_.append(fill.ring.size() >= 3 ? DrawUtils::triangulatePolygonXY(fill.ring) : QVector<QVector3D>());
    }
    strokeCache_ = std::make_shared<StrokeCache>();
}

void MissionLayer::MissionLayerRenderImplementation::clearData()
{
    data_ = RenderData{};
    fillTris_.clear();
    strokeCache_ = std::make_shared<StrokeCache>();
}

void MissionLayer::MissionLayerRenderImplementation::render(
    QOpenGLFunctions* ctx,
    const QMatrix4x4& model,
    const QMatrix4x4& view,
    const QMatrix4x4& projection,
    const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>>& shaderProgramMap) const
{
    if (!m_isVisible || !data_.enabled) {
        return;
    }

    const float scale = static_cast<float>(renderScale());
    const QRectF viewport = DrawUtils::viewportRect(ctx);

    if (auto it = shaderProgramMap.find("static"); it != shaderProgramMap.end() && it.value()) {
        const auto& sp = it.value();
        if (sp->bind()) {
            const int posLoc = sp->attributeLocation("position");
            const int colorLoc = sp->uniformLocation("color");
            sp->setUniformValue(sp->uniformLocation("matrix"), projection * view * model);
            sp->enableAttributeArray(posLoc);
            for (int i = 0; i < data_.fills.size() && i < fillTris_.size(); ++i) {
                const auto& tris = fillTris_[i];
                if (tris.isEmpty()) {
                    continue;
                }
                sp->setUniformValue(colorLoc, DrawUtils::colorToVector4d(data_.fills[i].color));
                sp->setAttributeArray(posLoc, tris.constData());
                ctx->glDrawArrays(GL_TRIANGLES, 0, tris.size());
            }
            sp->disableAttributeArray(posLoc);
            sp->release();
        }
    }

    auto it2 = shaderProgramMap.find("static_sec");
    if (it2 == shaderProgramMap.end() || !it2.value()) {
        return;
    }

    ScreenSpace ss;
    ss.halfW = viewport.width() * 0.5f;
    ss.halfH = viewport.height() * 0.5f;
    ss.viewModel = view * model;
    ss.projection = projection;
    ss.viewport = viewport.toRect();
    if (!ss.valid()) {
        return;
    }

    QOpenGLShaderProgram& sp = *it2.value();
    if (!sp.bind()) {
        return;
    }
    const int colorLoc = sp.uniformLocation("color");
    sp.enableAttributeArray(0);

    if (!data_.lines.isEmpty()) {
        StrokeCache& cache = *strokeCache_;
        const bool stale = !cache.valid || cache.scale != scale || cache.viewModel != ss.viewModel
                           || cache.projection != ss.projection || cache.viewport != ss.viewport;
        if (stale) {
            cache.valid = true;
            cache.scale = scale;
            cache.viewModel = ss.viewModel;
            cache.projection = ss.projection;
            cache.viewport = ss.viewport;
            cache.halo.clear();
            cache.colors.clear();
            std::map<QRgb, int> batchIndex;
            QVector<QVector2D> px;
            for (const auto& line : data_.lines) {
                if (line.points.size() < 2) {
                    continue;
                }
                px.clear();
                px.reserve(line.points.size());
                for (const auto& p : line.points) {
                    px.append(ss.window(p));
                }
                const float width = std::round(line.widthPx * scale);
                appendStroke(cache.halo, px, line.loop, width + kHaloWidthPx, kHaloZ, ss);
                const QRgb key = line.color.rgba();
                auto found = batchIndex.find(key);
                if (found == batchIndex.end()) {
                    cache.colors.append(StrokeBatch{line.color, {}});
                    found = batchIndex.emplace(key, cache.colors.size() - 1).first;
                }
                appendStroke(cache.colors[found->second].verts, px, line.loop, width, kLineZ, ss);
            }
        }

        GLint prevDepthFunc = GL_LESS;
        ctx->glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
        GLboolean prevDepthMask = GL_TRUE;
        ctx->glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
        const bool prevDepthTest = ctx->glIsEnabled(GL_DEPTH_TEST);
        ctx->glEnable(GL_DEPTH_TEST);
        ctx->glDepthFunc(GL_LESS);
        ctx->glDepthMask(GL_TRUE);

        drawTriangles3(ctx, sp, colorLoc, cache.halo, data_.lineHaloColor);
        for (const auto& b : cache.colors) {
            drawTriangles3(ctx, sp, colorLoc, b.verts, b.color);
        }

        ctx->glDepthMask(prevDepthMask);
        ctx->glDepthFunc(prevDepthFunc);
        if (!prevDepthTest) {
            ctx->glDisable(GL_DEPTH_TEST);
        }
    }

    const float markerLineWidth = std::round(kMarkerLineWidthPx * scale);
    const float haloLineWidth = markerLineWidth + kHaloWidthPx;

    if (!data_.arrows.isEmpty()) {
        QVector<QVector2D> arrowVerts;
        arrowVerts.reserve(data_.arrows.size() * 4);
        const float len = kArrowLenPx * scale;
        for (const auto& ar : data_.arrows) {
            const QVector2D a = ss.window(ar.a);
            const QVector2D b = ss.window(ar.b);
            const QVector2D d = b - a;
            const float segLen = d.length();
            if (segLen < len * kArrowMinSegmentFactor) {
                continue;
            }
            const QVector2D dir = d / segLen;
            const QVector2D mid = a + dir * (len * 0.5f);
            const QVector2D left(-dir.x() * kCos30 + dir.y() * kSin30, -dir.y() * kCos30 - dir.x() * kSin30);
            const QVector2D right(-dir.x() * kCos30 - dir.y() * kSin30, -dir.y() * kCos30 + dir.x() * kSin30);
            arrowVerts.push_back(ss.toNdc(mid));
            arrowVerts.push_back(ss.toNdc(mid + left * len));
            arrowVerts.push_back(ss.toNdc(mid));
            arrowVerts.push_back(ss.toNdc(mid + right * len));
        }
        draw2d(ctx, sp, colorLoc, extendSegments(arrowVerts, ss), data_.haloColor, GL_LINES, haloLineWidth);
        draw2d(ctx, sp, colorLoc, arrowVerts, data_.arrowColor, GL_LINES, markerLineWidth);
    }

    QVector<MarkerBatch> tris[2];
    QVector<MarkerBatch> lines[2];
    for (const auto& m : data_.markers) {
        const int layer = m.topmost ? 1 : 0;
        QVector2D c = ss.ndc(m.world);
        c += QVector2D(m.offsetPx.x() * scale / ss.halfW, -m.offsetPx.y() * scale / ss.halfH);
        const float sz = m.sizePx * scale;
        const float rx = sz / ss.halfW;
        const float ry = sz / ss.halfH;
        const QRgb rgba = m.color.rgba();
        switch (m.shape) {
        case Marker::Shape::Circle:  appendCircle(batchFor(tris[layer], rgba, m.halo), c, rx, ry, circleSegments(sz)); break;
        case Marker::Shape::Ring:    appendRing(batchFor(lines[layer], rgba, m.halo), c, rx, ry, circleSegments(sz)); break;
        case Marker::Shape::Plus:    appendPlus(batchFor(lines[layer], rgba, m.halo), c, rx, ry); break;
        case Marker::Shape::Cross:   appendCross(batchFor(lines[layer], rgba, m.halo), c, rx, ry); break;
        case Marker::Shape::Move:    appendMove(batchFor(lines[layer], rgba, m.halo), c, rx, ry); break;
        case Marker::Shape::Diamond: appendDiamond(batchFor(tris[layer], rgba, m.halo), c, rx, ry); break;
        }
    }

    auto drawMarkerLayer = [&](int layer) {
        for (const auto& b : tris[layer]) {
            draw2d(ctx, sp, colorLoc, b.verts, QColor::fromRgba(b.color), GL_TRIANGLES, 1.0f);
        }
        for (const auto& b : lines[layer]) {
            if (b.halo) {
                draw2d(ctx, sp, colorLoc, extendSegments(b.verts, ss), data_.haloColor, GL_LINES, haloLineWidth);
            }
        }
        for (const auto& b : lines[layer]) {
            draw2d(ctx, sp, colorLoc, b.verts, QColor::fromRgba(b.color), GL_LINES, markerLineWidth);
        }
    };

    drawMarkerLayer(0);
    sp.disableAttributeArray(0);
    sp.release();

    QVector<TextRenderer::Text2DItem> labels;
    for (const auto& m : data_.markers) {
        if (m.label.isEmpty()) {
            continue;
        }
        QVector2D screen = ss.window(m.world);
        screen.setY(viewport.height() - screen.y());
        screen += m.offsetPx * scale;
        screen += QVector2D((m.sizePx + kLabelOffsetPx) * scale, -(m.sizePx + kLabelOffsetPx) * scale);
        labels.append(TextRenderer::Text2DItem{m.label, 1.0f, screen, true});
    }
    if (!labels.isEmpty()) {
        QMatrix4x4 textProjection;
        textProjection.ortho(viewport.toRect());
        TextRenderer::instance().setColor(data_.labelColor);
        TextRenderer::instance().setBackgroundColor(data_.labelBackground);
        TextRenderer::instance().render2DBatch(labels, ctx, textProjection, shaderProgramMap);
        ctx->glEnable(GL_BLEND);
        ctx->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    if (sp.bind()) {
        sp.enableAttributeArray(0);
        drawMarkerLayer(1);
        sp.disableAttributeArray(0);
        sp.release();
    }
}

MissionLayer::MissionLayer(QObject* parent)
    : SceneObject(new MissionLayerRenderImplementation(), parent, QStringLiteral("MissionLayer"))
{
    setVisible(true);
}

SceneObject::SceneObjectType MissionLayer::type() const
{
    return SceneObject::SceneObjectType::Unknown;
}

void MissionLayer::setRenderData(MissionLayer::RenderData data)
{
    auto* r = dynamic_cast<MissionLayerRenderImplementation*>(m_renderImpl);
    if (!r) {
        return;
    }
    r->setRenderData(std::move(data));
    emit changed();
}

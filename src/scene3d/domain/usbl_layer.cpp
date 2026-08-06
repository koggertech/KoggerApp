#include "usbl_layer.h"

#include "themes.h"


namespace {

// A beacon reads as three marks that mean three different things, so they are deliberately
// different sizes: the surface ball is where it is ON THE CHART, the small ball is where it
// actually is IN THE WATER, and the line between them is the depth that separates them.
constexpr float kSurfaceBallPx = 22.0f;
constexpr float kDeepBallPx    = 9.0f;
constexpr float kHeadBallPx    = 24.0f;
constexpr float kHaloPx        = 3.0f;

constexpr float kTrackWidth    = 2.5f;
// Thinner than a track, because it is not a path: nothing travelled along it.
constexpr float kDropWidth     = 1.5f;

// The arrow is drawn ON the head's ball rather than protruding from it, so it takes the ball's
// own point size. baseScale sets both the base's height and its width in the shader (see
// usbl_arrow.fsh), and this value is the widest head that still sits inside the unit disc.
constexpr float kYawArrowScale = 0.45f;
constexpr float kYawArrowSize  = 0.82f;

constexpr float kMutedOpacity  = 0.45f;

constexpr float kDegToRad = 0.01745329252f;

// The halo. Every ball gets one, because a saturated dot on a satellite basemap can land on a
// patch of its own colour and vanish -- which is the one thing a position marker must not do.
const QVector4D kHaloColor(0.94f, 0.94f, 0.94f, 1.0f);

QVector4D toVec4(const QColor& c, float opacity)
{
    return QVector4D(static_cast<float>(c.redF()),
                     static_cast<float>(c.greenF()),
                     static_cast<float>(c.blueF()),
                     opacity);
}

// Ink for a mark drawn on top of a fill, picked by luminance for the same reason
// AppPalette.accentText is: the fill is the address's, and the address palette spans amber to
// slate. Same weights as the QML side so one beacon does not get white here and near-black there.
QVector4D inkOn(const QColor& fill, float opacity)
{
    const double l = 0.2126 * fill.redF() + 0.7152 * fill.greenF() + 0.0722 * fill.blueF();
    return l < 0.55 ? QVector4D(1.0f, 1.0f, 1.0f, opacity)
                    : QVector4D(0.08f, 0.13f, 0.17f, opacity);
}

} // namespace


UsblLayer::UsblLayer(QObject* parent) :
    SceneObject(new UsblLayerRenderImplementation, parent)
{
}

UsblLayer::~UsblLayer()
{
}

SceneObject::SceneObjectType UsblLayer::type() const
{
    return SceneObjectType::UsblLayer;
}

// `changed()` only, deliberately -- no boundsChanged.
//
// This layer's vertices are in the VIEW frame (they are re-projected against `viewLlaRef_` under
// the current projection), while every other object's bounds are in the DATASET frame. Merging
// them into the scene's bounding cube is only meaningful while those two references coincide, and
// nothing at the merge site can tell. The other half of it: a rebuild happens inside
// `synchronize()`, and boundsChanged there re-enters updateBounds -> updatePlaneGrid AFTER the
// plane grid's render impl has already been copied for this frame.
//
// So beacons do not widen `fitAllInView`. The survey extent is the boat and bottom tracks', which
// is where an operator expects "fit" to take them anyway.
void UsblLayer::setRenderData(RenderData data)
{
    RENDER_IMPL(UsblLayer)->setRenderData(std::move(data));

    Q_EMIT changed();
}

void UsblLayer::clear()
{
    setRenderData(RenderData());
}

void UsblLayer::UsblLayerRenderImplementation::setRenderData(RenderData data)
{
    data_ = std::move(data);
    updateBounds();
}

void UsblLayer::UsblLayerRenderImplementation::drawBall(QOpenGLFunctions* ctx,
                                                        QOpenGLShaderProgram* prog,
                                                        int posLoc, int colorLoc, int widthLoc,
                                                        const QVector3D& at, const QColor& fill,
                                                        float radiusPx, float opacity) const
{
    const QVector<QVector3D> one{ at };
    const float scale = static_cast<float>(renderScale());

    prog->setAttributeArray(posLoc, one.constData());

    prog->setUniformValue(colorLoc, QVector4D(kHaloColor.x(), kHaloColor.y(), kHaloColor.z(), opacity));
    prog->setUniformValue(widthLoc, (radiusPx + kHaloPx) * scale);
    ctx->glDrawArrays(GL_POINTS, 0, 1);

    prog->setUniformValue(colorLoc, toVec4(fill, opacity));
    prog->setUniformValue(widthLoc, radiusPx * scale);
    ctx->glDrawArrays(GL_POINTS, 0, 1);
}

void UsblLayer::UsblLayerRenderImplementation::render(QOpenGLFunctions* ctx, const QMatrix4x4& mvp,
                                                      const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>>& shaderProgramMap) const
{
    if (!m_isVisible) {
        return;
    }
    if (data_.beacons.isEmpty() && !data_.head.hasFix) {
        return;
    }

    auto shaderProgram = shaderProgramMap.value("static", nullptr);
    if (!shaderProgram) {
        qWarning() << "Shader program 'static' not found!";
        return;
    }
    auto arrowShaderProgram = shaderProgramMap.value("usbl_arrow", nullptr);

    const float scale = static_cast<float>(renderScale());

    // Muted nodes are dimmed rather than hidden, which needs real alpha -- the layer is drawn
    // with the depth test on and no blending, so it has to turn blending on for itself and put
    // it back. Same courtesy the rest of the renderer extends between passes.
    const GLboolean hadBlend = ctx->glIsEnabled(GL_BLEND);
    ctx->glEnable(GL_BLEND);
    ctx->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!shaderProgram->bind()) {
        if (!hadBlend) ctx->glDisable(GL_BLEND);
        return;
    }

    const int posLoc        = shaderProgram->attributeLocation("position");
    const int matrixLoc     = shaderProgram->uniformLocation("matrix");
    const int colorLoc      = shaderProgram->uniformLocation("color");
    const int widthLoc      = shaderProgram->uniformLocation("width");
    const int isPointLoc    = shaderProgram->uniformLocation("isPoint");
    const int isTriangleLoc = shaderProgram->uniformLocation("isTriangle");

    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setUniformValue(isTriangleLoc, false);
    shaderProgram->setUniformValue(isPointLoc, false);

    // ── tracks, and the drop lines ────────────────────────────────────────────
    // Under the depth test, so a beacon behind the bottom surface is occluded by it -- these
    // are geometry in the scene, unlike the balls below.
    for (const auto& b : data_.beacons) {
        const float opacity = b.active ? 1.0f : kMutedOpacity;

        if (b.track.size() > 1) {
            shaderProgram->setUniformValue(colorLoc, toVec4(b.color, opacity));
            shaderProgram->setAttributeArray(posLoc, b.track.constData());
            ctx->glLineWidth(kTrackWidth * scale);
            ctx->glDrawArrays(GL_LINE_STRIP, 0, b.track.size());
        }

        if (b.hasFix && b.hasDeep) {
            const QVector<QVector3D> drop{ b.surface, b.deep };
            shaderProgram->setUniformValue(colorLoc, toVec4(b.color, opacity));
            shaderProgram->setAttributeArray(posLoc, drop.constData());
            ctx->glLineWidth(kDropWidth * scale);
            ctx->glDrawArrays(GL_LINES, 0, 2);
        }
    }

    if (data_.head.track.size() > 1) {
        shaderProgram->setUniformValue(colorLoc, QVector4D(0.85f, 0.87f, 0.90f, 1.0f));
        shaderProgram->setAttributeArray(posLoc, data_.head.track.constData());
        ctx->glLineWidth(kTrackWidth * scale);
        ctx->glDrawArrays(GL_LINE_STRIP, 0, data_.head.track.size());
    }

    ctx->glLineWidth(1.0f);

    // ── the balls ─────────────────────────────────────────────────────────────
    // Depth writing off and the depth test forced to pass: a position marker that disappears
    // under the bottom mesh is worse than useless, because its absence reads as "no fix".
    ctx->glEnable(34370); // GL_PROGRAM_POINT_SIZE
    ctx->glEnable(34913); // GL_POINT_SPRITE

    GLint prevDepthFunc = GL_LESS;
    GLboolean prevDepthMask = GL_TRUE;
    ctx->glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
    ctx->glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
    ctx->glDepthFunc(GL_ALWAYS);
    ctx->glDepthMask(GL_FALSE);

    shaderProgram->setUniformValue(isPointLoc, true);

    for (const auto& b : data_.beacons) {
        if (!b.hasFix) {
            continue;
        }
        const float opacity = b.active ? 1.0f : kMutedOpacity;

        if (b.hasDeep) {
            drawBall(ctx, shaderProgram.get(), posLoc, colorLoc, widthLoc,
                     b.deep, b.color, kDeepBallPx, opacity);
        }
        drawBall(ctx, shaderProgram.get(), posLoc, colorLoc, widthLoc,
                 b.surface, b.color, kSurfaceBallPx, opacity);
    }

    if (data_.head.hasFix) {
        drawBall(ctx, shaderProgram.get(), posLoc, colorLoc, widthLoc,
                 data_.head.pos, QColor(60, 70, 82), kHeadBallPx, 1.0f);
    }

    shaderProgram->setUniformValue(isPointLoc, false);
    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();

    // ── the head's heading ────────────────────────────────────────────────────
    // Drawn last and on top of its own ball. This is the ACOUSTIC head's yaw, not the boat's:
    // NavigationArrow marks the same patch of water from the GNSS/IMU heading, and the two
    // disagreeing by a mounting offset is a thing worth being able to see.
    if (arrowShaderProgram && data_.head.hasFix && data_.head.hasYaw && arrowShaderProgram->bind()) {
        const QVector<QVector3D> one{ data_.head.pos };

        const int aPos   = arrowShaderProgram->attributeLocation("position");
        arrowShaderProgram->setUniformValue(arrowShaderProgram->uniformLocation("matrix"), mvp);
        arrowShaderProgram->setUniformValue(arrowShaderProgram->uniformLocation("yaw"),
                                            data_.head.yawDeg * kDegToRad);
        arrowShaderProgram->setUniformValue(arrowShaderProgram->uniformLocation("baseScale"),
                                            kYawArrowScale);
        arrowShaderProgram->setUniformValue(arrowShaderProgram->uniformLocation("color"),
                                            inkOn(QColor(60, 70, 82), 1.0f));
        arrowShaderProgram->setUniformValue(arrowShaderProgram->uniformLocation("width"),
                                            kHeadBallPx * kYawArrowSize * scale);
        arrowShaderProgram->enableAttributeArray(aPos);
        arrowShaderProgram->setAttributeArray(aPos, one.constData());
        ctx->glDrawArrays(GL_POINTS, 0, 1);
        arrowShaderProgram->disableAttributeArray(aPos);
        arrowShaderProgram->release();
    }

    ctx->glDepthMask(prevDepthMask);
    ctx->glDepthFunc(prevDepthFunc);
    ctx->glDisable(34370);
    ctx->glDisable(34913);

    if (!hadBlend) {
        ctx->glDisable(GL_BLEND);
    }
}

void UsblLayer::UsblLayerRenderImplementation::updateBounds()
{
    m_bounds = Cube();

    bool any = false;
    float xMin = 0.0f, xMax = 0.0f, yMin = 0.0f, yMax = 0.0f, zMin = 0.0f, zMax = 0.0f;

    // Every point here came from a finite lat/lon -- the controller drops a fix without one --
    // so unlike the layer this replaces there is no NAN to defend against.
    auto take = [&](const QVector3D& p) {
        if (!any) {
            xMin = xMax = p.x();
            yMin = yMax = p.y();
            zMin = zMax = p.z();
            any = true;
            return;
        }
        xMin = std::min(xMin, p.x()); xMax = std::max(xMax, p.x());
        yMin = std::min(yMin, p.y()); yMax = std::max(yMax, p.y());
        zMin = std::min(zMin, p.z()); zMax = std::max(zMax, p.z());
    };

    for (const auto& b : data_.beacons) {
        for (const auto& p : b.track) {
            take(p);
        }
        if (b.hasDeep) {
            take(b.deep);
        }
    }
    for (const auto& p : data_.head.track) {
        take(p);
    }

    if (any) {
        m_bounds = Cube(xMin, xMax, yMin, yMax, zMin, zMax);
    }
}

#include "usbl_layer.h"

#include <utility>

#include "draw_utils.h"
#include "text_renderer.h"
#include "themes.h"


namespace {

// A beacon reads as three marks that mean three different things, so they are deliberately
// different sizes: the surface ball is where it is ON THE CHART, the small ball is where it
// actually is IN THE WATER, and the line between them is the depth that separates them.
constexpr float kSurfaceBallPx = 11.0f;
constexpr float kDeepBallPx    = 4.5f;
// Twice the beacons': it carries the yaw dart, and a dart inside an 11 px disc is a smudge.
constexpr float kHeadBallPx    = 24.0f;
// An OUTLINE, so it does not scale with the ball -- a contour that grew with the fill would read
// as a second ring rather than as an edge. This is a DIAMETER increment: half of it is the ring's
// visible thickness on each side.
constexpr float kHaloPx        = 1.5f;

constexpr float kTrackWidth    = 5.0f;
// Thinner than a track, because it is not a path: nothing travelled along it.
constexpr float kDropWidth     = 1.5f;

// The heading marker is a paper dart drawn ON the head's ball, not protruding from it. Its
// half-span is what makes it narrow; its length is fixed in the shader. The sprite is sized just
// under the ball so the whole dart sits inside the disc with a hair of margin -- the shape's
// furthest corner is at ~0.96 of the sprite radius, so 0.90 leaves it clear of the rim.
constexpr float kYawDartHalfWidth = 0.42f;
constexpr float kYawDartSize      = 0.90f;

// A dot marking one fix. Smaller than the surface ball, because a track of them must read as a
// trail rather than as a row of beacons.
constexpr float kTrackDotPx    = 5.0f;

// The address label sits clear of the ball's rim rather than centred on it: centred, the digit
// would fight the yaw dart on the head and the depth drop line on a beacon.
constexpr float kLabelOffsetPx = 9.0f;
constexpr float kLabelScale    = 1.0f;

// ONE colour for every address label, not the beacon's own. The ball already carries the
// identity colour; repeating it in the digit made the label compete with the mark instead of
// naming it, and the paler addresses lost the digit against the chart. Amber is taken from the
// address palette (UsblFieldLogic.ADDRESS_COLORS, where it is address 3) rather than invented
// here -- a swatch borrowed from the set, not a second copy of the address-to-colour rule.
const QColor kLabelColor{0xE0, 0x90, 0x2B};

// The same casing the compass labels wear (CoordinateAxes), white here rather than dark because
// the digit is amber: the ring has to be the opposite side of the basemap from the glyph, and a
// basemap runs light. Eight offsets on a unit circle, so the ring closes instead of showing
// corners; TextRenderer draws one colour at a time, so a ring is nine draws of the same glyph.
constexpr float kLabelCasingPx = 1.0f;
const QColor kLabelCasingColor{255, 255, 255};

const QVector2D kLabelCasingOffsets[] = {
    QVector2D( 1.000f,  0.000f),
    QVector2D(-1.000f,  0.000f),
    QVector2D( 0.000f,  1.000f),
    QVector2D( 0.000f, -1.000f),
    QVector2D( 0.707f,  0.707f),
    QVector2D( 0.707f, -0.707f),
    QVector2D(-0.707f,  0.707f),
    QVector2D(-0.707f, -0.707f)
};

constexpr float kDegToRad = 0.01745329252f;

QVector4D toVec4(const QColor& c, float opacity)
{
    return QVector4D(static_cast<float>(c.redF()),
                     static_cast<float>(c.greenF()),
                     static_cast<float>(c.blueF()),
                     opacity);
}

// The contour every ball gets, because a saturated dot on a satellite basemap can land on a patch
// of its own colour and vanish -- which is the one thing a position marker must not do.
//
// It is a DARKENED VERSION OF THE BALL rather than a neutral light ring. White separated the mark
// from the chart but also from the beacon: eight addresses in eight colours all wearing the same
// bright halo, and at a glance the halo is what you see. Darkening the fill keeps the ring reading
// as that beacon's edge, and dark holds its own against a basemap that is mostly mid-tones and
// sunlight.
QColor contourOf(const QColor& fill)
{
    return fill.darker(250);
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

void UsblLayer::setBeaconTrackStyle(TrackStyle style)
{
    RENDER_IMPL(UsblLayer)->beaconTrackStyle_ = style;

    Q_EMIT changed();
}

void UsblLayer::setHeadTrackStyle(TrackStyle style)
{
    RENDER_IMPL(UsblLayer)->headTrackStyle_ = style;

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

    prog->setUniformValue(colorLoc, toVec4(contourOf(fill), opacity));
    prog->setUniformValue(widthLoc, (radiusPx + kHaloPx) * scale);
    ctx->glDrawArrays(GL_POINTS, 0, 1);

    prog->setUniformValue(colorLoc, toVec4(fill, opacity));
    prog->setUniformValue(widthLoc, radiusPx * scale);
    ctx->glDrawArrays(GL_POINTS, 0, 1);
}

void UsblLayer::UsblLayerRenderImplementation::render(QOpenGLFunctions* ctx, const QMatrix4x4& model,
                                                      const QMatrix4x4& view, const QMatrix4x4& projection,
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

    const QMatrix4x4 mvp = projection * view * model;
    const float scale = static_cast<float>(renderScale());

    // The layer is drawn with the depth test on and no blending, so it turns blending on for
    // itself and puts it back -- the label backgrounds and the round point sprites both need it.
    // Same courtesy the rest of the renderer extends between passes.
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

    // Needed from here on and not only for the balls: a dotted track is drawn as point sprites
    // too, and this pass comes first.
    ctx->glEnable(34370); // GL_PROGRAM_POINT_SIZE
    ctx->glEnable(34913); // GL_POINT_SPRITE

    // One track, drawn either as the path travelled or as the fixes that make it up. Shared so
    // the head and the beacons cannot drift apart in width, in dot size, or in which uniform is
    // left set behind them.
    auto drawTrack = [&](const QVector<QVector3D>& track, TrackStyle style) {
        shaderProgram->setAttributeArray(posLoc, track.constData());
        if (style == TrackStyle::Dots) {
            shaderProgram->setUniformValue(isPointLoc, true);
            shaderProgram->setUniformValue(widthLoc, kTrackDotPx * scale);
            ctx->glDrawArrays(GL_POINTS, 0, track.size());
            shaderProgram->setUniformValue(isPointLoc, false);
        }
        else {
            ctx->glLineWidth(kTrackWidth * scale);
            ctx->glDrawArrays(GL_LINE_STRIP, 0, track.size());
        }
    };

    // ── tracks, and the drop lines ────────────────────────────────────────────
    // Under the depth test, so a beacon behind the bottom surface is occluded by it -- these
    // are geometry in the scene, unlike the balls below.
    //
    // AN INACTIVE NODE IS NOT DRAWN AT ALL. It used to be dimmed, on the reasoning that a beacon
    // switched off is still a beacon that exists -- but the switch is how the map gets cleared,
    // and a ghost that still covers the chart does not clear it.
    for (const auto& b : data_.beacons) {
        if (!b.active) {
            continue;
        }

        if (b.track.size() > 1) {
            shaderProgram->setUniformValue(colorLoc, toVec4(b.color, 1.0f));
            drawTrack(b.track, beaconTrackStyle_);
        }

        if (b.hasFix && b.hasDeep) {
            const QVector<QVector3D> drop{ b.surface, b.deep };
            shaderProgram->setUniformValue(colorLoc, toVec4(b.color, 1.0f));
            shaderProgram->setAttributeArray(posLoc, drop.constData());
            ctx->glLineWidth(kDropWidth * scale);
            ctx->glDrawArrays(GL_LINES, 0, 2);
        }
    }

    if (data_.head.track.size() > 1) {
        shaderProgram->setUniformValue(colorLoc, QVector4D(0.85f, 0.87f, 0.90f, 1.0f));
        drawTrack(data_.head.track, headTrackStyle_);
    }

    ctx->glLineWidth(1.0f);

    // ── the balls ─────────────────────────────────────────────────────────────
    // Depth writing off and the depth test forced to pass: a position marker that disappears
    // under the bottom mesh is worse than useless, because its absence reads as "no fix".
    GLint prevDepthFunc = GL_LESS;
    GLboolean prevDepthMask = GL_TRUE;
    ctx->glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
    ctx->glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
    ctx->glDepthFunc(GL_ALWAYS);
    ctx->glDepthMask(GL_FALSE);

    shaderProgram->setUniformValue(isPointLoc, true);

    for (const auto& b : data_.beacons) {
        if (!b.active || !b.hasFix) {
            continue;
        }

        if (b.hasDeep) {
            drawBall(ctx, shaderProgram.get(), posLoc, colorLoc, widthLoc,
                     b.deep, b.color, kDeepBallPx, 1.0f);
        }
        drawBall(ctx, shaderProgram.get(), posLoc, colorLoc, widthLoc,
                 b.surface, b.color, kSurfaceBallPx, 1.0f);
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
    //
    // `usbl_yaw` is passed straight through as a COMPASS BEARING in degrees -- 0 = north, turning
    // clockwise -- and usbl_arrow.fsh is written to that convention. The layer this replaced fed
    // the same shader a hard-coded 0.0f and 90.0f, so no version of this code has ever put a
    // measured heading through it; if the dart points somewhere the head does not, the sign or
    // the zero is what to question, and both live in the shader.
    if (arrowShaderProgram && data_.head.hasFix && data_.head.hasYaw && arrowShaderProgram->bind()) {
        const QVector<QVector3D> one{ data_.head.pos };

        const int aPos   = arrowShaderProgram->attributeLocation("position");
        arrowShaderProgram->setUniformValue(arrowShaderProgram->uniformLocation("matrix"), mvp);
        arrowShaderProgram->setUniformValue(arrowShaderProgram->uniformLocation("yaw"),
                                            data_.head.yawDeg * kDegToRad);
        arrowShaderProgram->setUniformValue(arrowShaderProgram->uniformLocation("halfWidth"),
                                            kYawDartHalfWidth);
        arrowShaderProgram->setUniformValue(arrowShaderProgram->uniformLocation("color"),
                                            inkOn(QColor(60, 70, 82), 1.0f));
        arrowShaderProgram->setUniformValue(arrowShaderProgram->uniformLocation("width"),
                                            kHeadBallPx * kYawDartSize * scale);
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

    // ── the addresses ─────────────────────────────────────────────
    // On screen rather than in the world: a label that tilted and scaled with the scene would be
    // unreadable at exactly the zoom where the beacons collapse into one cluster, and that is the
    // zoom where which ball is which is the whole question.
    //
    // One render() per label rather than a batch: the batch takes the singleton's colour for all
    // items, and restoring that colour once around the loop is the same bookkeeping either way.
    // There are at most eight addresses, so the difference is not measurable.
    //
    // NO BACKGROUND PLATE. TextRenderer is a singleton whose colour AND background colour are
    // whatever the last caller left behind -- RulerTool leaves black at 160 alpha -- so a plate
    // here came out as a black box nobody asked for. The digit sits next to its own ball, which
    // is the contrast it needs; the colour is saved and put back for the same reason.
    {
        const QRectF vport = DrawUtils::viewportRect(ctx);
        QMatrix4x4 textProjection;
        textProjection.ortho(vport.toRect());

        const QColor prevTextColor = TextRenderer::instance().getColor();
        const float off = kLabelOffsetPx * scale;
        const float casingPx = qMax(1.0f, std::round(kLabelCasingPx * scale));

        for (const auto& b : data_.beacons) {
            if (!b.active || !b.hasFix || b.addr < 0) {
                continue;
            }

            QVector2D at = b.surface.project(view * model, projection, vport.toRect()).toVector2D();
            at.setX(at.x() + off);
            at.setY(static_cast<float>(vport.height()) - at.y() + off);

            const QString text = QString::number(b.addr);

            TextRenderer::instance().setColor(kLabelCasingColor);
            for (const auto& casingOffset : kLabelCasingOffsets) {
                TextRenderer::instance().render(text, kLabelScale, at + casingOffset * casingPx,
                                                false, ctx, textProjection, shaderProgramMap);
            }

            TextRenderer::instance().setColor(kLabelColor);
            TextRenderer::instance().render(text, kLabelScale, at, false,
                                            ctx, textProjection, shaderProgramMap);
        }

        TextRenderer::instance().setColor(prevTextColor);
    }

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

    for (const auto& b : std::as_const(data_.beacons)) {
        for (const auto& p : b.track) {
            take(p);
        }
        if (b.hasDeep) {
            take(b.deep);
        }
    }
    for (const auto& p : std::as_const(data_.head.track)) {
        take(p);
    }

    if (any) {
        m_bounds = Cube(xMin, xMax, yMin, yMax, zMin, zMax);
    }
}

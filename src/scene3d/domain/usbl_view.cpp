#include "usbl_view.h"


UsblView::UsblView(QObject* parent) :
    SceneObject(new UsblViewRenderImplementation, parent)
{

}

UsblView::~UsblView()
{

}

SceneObject::SceneObjectType UsblView::type() const
{
    return SceneObjectType::UsblView;
}

void UsblView::UsblViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp,
                                                    const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible) {
        return;
    }

    auto shaderProgram = shaderProgramMap.value("static", nullptr);
    auto arrowShaderProgram = shaderProgramMap.value("usbl_arrow", nullptr);

    if (!shaderProgram) {
        qWarning() << "Shader program 'static' not found!";
        return;
    }

    shaderProgram->bind();

    int posLoc        = shaderProgram->attributeLocation ("position");
    int matrixLoc     = shaderProgram->uniformLocation   ("matrix");
    int colorLoc      = shaderProgram->uniformLocation   ("color");
    int widthLoc      = shaderProgram->uniformLocation   ("width");
    int isPointLoc    = shaderProgram->uniformLocation   ("isPoint");
    int isTriangleLoc = shaderProgram->uniformLocation   ("isTriangle");

    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setUniformValue(isTriangleLoc, false);

    auto subPointColor = QVector4D(0.9f, 0.9f, 0.9f, 1.0f);
    constexpr float kDegToRad = 0.01745329252f;
    constexpr float kArrowBaseScale = 0.8f;
    constexpr float kArrowTipScale = 1.9f;
    constexpr float kArrowHaloPaddingPx = 6.0f;

    int arrowPosLoc = -1;
    int arrowMatrixLoc = -1;
    int arrowColorLoc = -1;
    int arrowWidthLoc = -1;
    int arrowYawLoc = -1;
    int arrowBaseScaleLoc = -1;
    if (arrowShaderProgram) {
        arrowPosLoc = arrowShaderProgram->attributeLocation("position");
        arrowMatrixLoc = arrowShaderProgram->uniformLocation("matrix");
        arrowColorLoc = arrowShaderProgram->uniformLocation("color");
        arrowWidthLoc = arrowShaderProgram->uniformLocation("width");
        arrowYawLoc = arrowShaderProgram->uniformLocation("yaw");
        arrowBaseScaleLoc = arrowShaderProgram->uniformLocation("baseScale");
    }

    for (auto &itm : tracks_) {
        if (itm.type_ == UsblView::UsblObjectType::kUndefined) {
            continue;
        }

        auto lineColor = QVector4D(itm.objectColor_.redF(), itm.objectColor_.greenF(), itm.objectColor_.blueF(), 1.0f);
        if (itm.isTrackVisible_) {
            shaderProgram->setUniformValue(colorLoc, lineColor);
            shaderProgram->setUniformValue(widthLoc, itm.lineWidth_);
            shaderProgram->setAttributeArray(posLoc, itm.data_.constData());
            ctx->glLineWidth(itm.lineWidth_);
            ctx->glDrawArrays(GL_LINE_STRIP, 0, itm.data_.size());
            ctx->glLineWidth(1.0f);
        }
    }

    ctx->glEnable(34370);
    ctx->glEnable(34913);
    GLint prevDepthFunc = GL_LESS;
    GLboolean prevDepthMask = GL_TRUE;
    ctx->glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
    ctx->glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
    ctx->glDepthFunc(GL_ALWAYS);
    ctx->glDepthMask(GL_FALSE);

    for (auto &itm : tracks_) {
        if (itm.type_ == UsblView::UsblObjectType::kUndefined) {
            continue;
        }

        auto lineColor = QVector4D(itm.objectColor_.redF(), itm.objectColor_.greenF(), itm.objectColor_.blueF(), 1.0f);
        bool hasYaw = arrowShaderProgram && std::isfinite(itm.yaw_);
        float pointScale = hasYaw ? kArrowBaseScale : 1.0f;
        float arrowScale = hasYaw ? kArrowTipScale : 1.0f;
        float invPointRadius = (itm.pointRadius_ > 0.0f) ? (1.0f / itm.pointRadius_) : 0.0f;
        //float arrowBaseScale = (arrowScale > 0.0f) ? (pointScale / arrowScale) : pointScale;
        float arrowHaloScale = arrowScale + kArrowHaloPaddingPx * invPointRadius;
        float arrowHaloBaseScale = (arrowHaloScale > 0.0f) ? (pointScale / arrowHaloScale) : pointScale;

        // point
        bool isUsbl = itm.type_ == UsblView::UsblObjectType::kUsbl ? true : false; // true - circle, false - square

        QVector<QVector3D> point{ itm.data_.last() };
        //qDebug() << "arrowShaderProgram" << arrowShaderProgram << itm.yaw_;
        if (arrowShaderProgram && hasYaw) {
            if (arrowShaderProgram->bind()) {
                arrowShaderProgram->setUniformValue(arrowMatrixLoc, mvp);
                arrowShaderProgram->setUniformValue(arrowYawLoc, itm.yaw_ * kDegToRad);

                // arrow halo (behind point halo)
                arrowShaderProgram->setUniformValue(arrowBaseScaleLoc, arrowHaloBaseScale);
                arrowShaderProgram->setUniformValue(arrowColorLoc, subPointColor);
                arrowShaderProgram->setUniformValue(arrowWidthLoc, itm.pointRadius_ * arrowHaloScale);
                arrowShaderProgram->enableAttributeArray(arrowPosLoc);
                arrowShaderProgram->setAttributeArray(arrowPosLoc, point.constData());
                ctx->glDrawArrays(GL_POINTS, 0, point.size());

                //// arrow color (behind point)
                //arrowShaderProgram->setUniformValue(arrowBaseScaleLoc, kArrowBaseScale);
                //arrowShaderProgram->setUniformValue(arrowColorLoc, lineColor);
                //arrowShaderProgram->setUniformValue(arrowWidthLoc, itm.pointRadius_ * arrowScale);
                //ctx->glDrawArrays(GL_POINTS, 0, point.size());

                arrowShaderProgram->disableAttributeArray(arrowPosLoc);
                arrowShaderProgram->release();

                shaderProgram->bind();
                shaderProgram->setUniformValue(matrixLoc, mvp);
                shaderProgram->enableAttributeArray(posLoc);
                shaderProgram->setUniformValue(isTriangleLoc, false);
            }
        }

        // point halo + point color (always)
        shaderProgram->setUniformValue(isPointLoc, isUsbl);
        shaderProgram->setUniformValue(colorLoc, subPointColor);
        shaderProgram->setUniformValue(widthLoc, itm.pointRadius_ * pointScale + 3.0f);
        shaderProgram->setAttributeArray(posLoc, point.constData());
        ctx->glDrawArrays(GL_POINTS, 0, point.size());
        shaderProgram->setUniformValue(colorLoc, lineColor);
        shaderProgram->setUniformValue(widthLoc, itm.pointRadius_ * pointScale);
        shaderProgram->setAttributeArray(posLoc, point.constData());
        ctx->glDrawArrays(GL_POINTS, 0, point.size());
        shaderProgram->setUniformValue(isPointLoc, false);
    }

    ctx->glDepthMask(prevDepthMask);
    ctx->glDepthFunc(prevDepthFunc);
    ctx->glDisable(34370); // GL_PROGRAM_POINT_SIZE
    ctx->glDisable(34913); // GL_POINT_SPRITE

    shaderProgram->setUniformValue(isPointLoc, false);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

void UsblView::UsblViewRenderImplementation::updateBounds()
{
    m_bounds = Cube();

    if (tracks_.isEmpty()) {
        return;
    }

    for (auto &itmI : tracks_) {
        Cube curr_bounds;

        float z_max{ !std::isfinite(itmI.data_.first().z()) ? 0.f : itmI.data_.first().z() };
        float z_min{ z_max };
        float x_max{ !std::isfinite(itmI.data_.first().x()) ? 0.f : itmI.data_.first().x() };
        float x_min{ x_max };
        float y_max{ !std::isfinite(itmI.data_.first().y()) ? 0.f : itmI.data_.first().y() };
        float y_min{ y_max };

        for (auto it = itmI.data_.cbegin(); it != itmI.data_.cend(); ++it) {
            z_min = std::min(z_min, !std::isfinite(it->z()) ? 0.f : it->z());
            z_max = std::max(z_max, !std::isfinite(it->z()) ? 0.f : it->z());
            x_min = std::min(x_min, !std::isfinite(it->x()) ? 0.f : it->x());
            x_max = std::max(x_max, !std::isfinite(it->x()) ? 0.f : it->x());
            y_min = std::min(y_min, !std::isfinite(it->y()) ? 0.f : it->y());
            y_max = std::max(y_max, !std::isfinite(it->y()) ? 0.f : it->y());
        }

        curr_bounds = Cube(x_min, x_max, y_min, y_max, z_min, z_max);

        m_bounds = m_bounds.merge(curr_bounds);
    }
}

void UsblView::setTrackRef(QMap<int, UsblObjectParams>& tracks)
{
    // check data
    bool beenRefreshed{ false };
    for (auto it = tracks.begin(); it != tracks.end(); ++it) {
        int key = it.key();
        UsblObjectParams& newParams = it.value();

        if (tracks_.contains(key)) {
            UsblObjectParams& currentParams = tracks_[key];

            if (currentParams.isTrackVisible_ != newParams.isTrackVisible_) {
                currentParams.isTrackVisible_ = newParams.isTrackVisible_;
                beenRefreshed = true;
            }
            if (currentParams.type_ != newParams.type_) {
                currentParams.type_ = newParams.type_;
                beenRefreshed = true;
            }
            if (currentParams.lineWidth_ != newParams.lineWidth_) {
                currentParams.lineWidth_ = newParams.lineWidth_;
                beenRefreshed = true;
            }
            if (currentParams.pointRadius_ != newParams.pointRadius_) {
                currentParams.pointRadius_ = newParams.pointRadius_;
                beenRefreshed = true;
            }
            if (currentParams.objectColor_ != newParams.objectColor_) {
                currentParams.objectColor_ = newParams.objectColor_;
                beenRefreshed = true;
            }
            if (currentParams.data_.size() != newParams.data_.size()) {

                if (newParams.data_.size() > currentParams.data_.size()) {
                    for (int i = currentParams.data_.size(); i < newParams.data_.size(); ++i) {
                        currentParams.data_.append(newParams.data_[i]);
                    }
                }
                else if (newParams.data_.size() < currentParams.data_.size()) {
                    currentParams.data_ = newParams.data_;
                }
                beenRefreshed = true;
            }
        }
        else {
            tracks_.insert(key, newParams);
            beenRefreshed = true;
        }
    }

    for (auto it = tracks_.begin(); it != tracks_.end();) {
        if (!tracks.contains(it.key())) {
            it = tracks_.erase(it);
            beenRefreshed = true;
        }
        else {
            ++it;
        }
    }

    // refresh in render only needed
    if (beenRefreshed) {
        RENDER_IMPL(UsblView)->tracks_ = tracks_;
        RENDER_IMPL(UsblView)->updateBounds();

        Q_EMIT changed();
        Q_EMIT boundsChanged();
    }
}

void UsblView::clearTracks()
{
    tracks_.clear();
    RENDER_IMPL(UsblView)->tracks_.clear();
    RENDER_IMPL(UsblView)->updateBounds();

    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

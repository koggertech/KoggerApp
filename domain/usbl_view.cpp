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

    if (!shaderProgram) {
        qWarning() << "Shader program 'static' not found!";
        return;
    }

    shaderProgram->bind();

    int posLoc     = shaderProgram->attributeLocation ("position");
    int matrixLoc  = shaderProgram->uniformLocation   ("matrix");
    int colorLoc   = shaderProgram->uniformLocation   ("color");
    int widthLoc   = shaderProgram->uniformLocation   ("width");
    int isPointLoc = shaderProgram->uniformLocation   ("isPoint");

    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);

    auto subPointColor = QVector4D(0.9f, 0.9f, 0.9f, 1.0f);

    for (auto &itm : tracks_) {
        auto lineColor = QVector4D(itm.objectColor_.redF(), itm.objectColor_.greenF(), itm.objectColor_.blueF(), 1.0f);

        // point
        bool isUsbl = itm.type_ == UsblView::UsblObjectType::kUsbl ? true : false;

#ifndef Q_OS_ANDROID
        ctx->glEnable(GL_PROGRAM_POINT_SIZE);
#else
        ctx->glEnable(34370);
#endif

        shaderProgram->setUniformValue(isPointLoc, !isUsbl);
        QVector<QVector3D> point{ itm.data_.last() };
        // color point
        shaderProgram->setUniformValue(colorLoc, lineColor);
        shaderProgram->setUniformValue(widthLoc, itm.pointRadius_);
        shaderProgram->setAttributeArray(posLoc, point.constData());
        ctx->glDrawArrays(GL_POINTS, 0, point.size());
        // gray point
        shaderProgram->setUniformValue(colorLoc, subPointColor);
        shaderProgram->setUniformValue(widthLoc, itm.pointRadius_ + 3.0f);
        shaderProgram->setAttributeArray(posLoc, point.constData());
        ctx->glDrawArrays(GL_POINTS, 0, point.size());
        shaderProgram->setUniformValue(isPointLoc, false);

#ifndef Q_OS_ANDROID
        ctx->glDisable(GL_PROGRAM_POINT_SIZE);
#else
        ctx->glDisable(34370);
#endif

        // line
        if (itm.isTrackVisible_) {
            shaderProgram->setUniformValue(colorLoc, lineColor);
            shaderProgram->setUniformValue(widthLoc, itm.lineWidth_);
            shaderProgram->setAttributeArray(posLoc, itm.data_.constData());
            ctx->glLineWidth(itm.lineWidth_);
            ctx->glDrawArrays(GL_LINE_STRIP, 0, itm.data_.size());
            ctx->glLineWidth(1.0f);
        }
    }

    shaderProgram->setUniformValue(isPointLoc, false);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

void UsblView::UsblViewRenderImplementation::createBounds()
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

        for (const auto& itmJ : qAsConst(itmI.data_)){
            z_min = std::min(z_min, !std::isfinite(itmJ.z()) ? 0.f : itmJ.z());
            z_max = std::max(z_max, !std::isfinite(itmJ.z()) ? 0.f : itmJ.z());
            x_min = std::min(x_min, !std::isfinite(itmJ.x()) ? 0.f : itmJ.x());
            x_max = std::max(x_max, !std::isfinite(itmJ.x()) ? 0.f : itmJ.x());
            y_min = std::min(y_min, !std::isfinite(itmJ.y()) ? 0.f : itmJ.y());
            y_max = std::max(y_max, !std::isfinite(itmJ.y()) ? 0.f : itmJ.y());
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
        RENDER_IMPL(UsblView)->createBounds();

        Q_EMIT changed();
        Q_EMIT boundsChanged();
    }
}

void UsblView::clearTracks()
{
    tracks_.clear();
    RENDER_IMPL(UsblView)->tracks_.clear();
    RENDER_IMPL(UsblView)->createBounds();

    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

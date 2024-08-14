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

void UsblView::setData(const QVector<QVector3D>& data, int primitiveType)
{
    SceneObject::setData(data, primitiveType);

    Q_EMIT changed();
}

void UsblView::clearData()
{
    SceneObject::clearData();
}

void UsblView::UsblViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp,
                                                    const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible) {
        return;
    }

    auto shaderProgram = shaderProgramMap.value("static", nullptr);

    if (!shaderProgram) {
        qWarning() << "Shader program 'mosaic' not found!";
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

        //qDebug() << itm.data_ ;

        auto lineColor = QVector4D(itm.objectColor_.redF(), itm.objectColor_.greenF(), itm.objectColor_.blueF(), 1.0f);

        // point
        bool isUsbl = itm.type_ == UsblView::UsblObjectType::kUsbl ? true : false;

        ctx->glEnable(GL_PROGRAM_POINT_SIZE);
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
        ctx->glDisable(GL_PROGRAM_POINT_SIZE);

        if (itm.isTrackVisible_) {
            shaderProgram->setUniformValue(colorLoc, lineColor);
            shaderProgram->setUniformValue(isPointLoc, false);
            shaderProgram->setUniformValue(widthLoc, m_width);

            shaderProgram->setAttributeArray(posLoc, itm.data_.constData());
            ctx->glLineWidth(m_width);
            ctx->glDrawArrays(GL_LINE_STRIP, 0, itm.data_.size());
            ctx->glLineWidth(1.0f);
        }
    }

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
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
        Q_EMIT changed();
    }
}

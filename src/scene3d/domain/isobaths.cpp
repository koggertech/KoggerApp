#include "isobaths.h"

#include "text_renderer.h"


Isobaths::Isobaths(QObject* parent)
    : SceneObject(new IsobathsRenderImplementation, parent)
{}

Isobaths::~Isobaths()
{}

void Isobaths::clear()
{
    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->pts_.clear();
        r->edgePts_.clear();
        r->lineSegments_.clear();
        r->labels_.clear();
    }

    Q_EMIT changed();
}

void Isobaths::setCameraDistToFocusPoint(float val)
{
    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->distToFocusPoint_ = val;
    }
}

void Isobaths::setLabels(const QVector<LabelParameters> &labels)
{
    //qDebug() << "Isobaths::setLabels" << labels.size();

    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->labels_ = labels;
        Q_EMIT changed();
    }
}

void Isobaths::setLineSegments(const QVector<QVector3D> &lineSegments)
{
    //qDebug() << "Isobaths::setLineSegments" << lineSegments.size();

    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->lineSegments_ = lineSegments;
        Q_EMIT changed();
    }
}

void Isobaths::setPts(const QVector<QVector3D> &pts)
{
    //qDebug() << "Isobaths::setPts" << pts;

    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->pts_ = pts;
        Q_EMIT changed();
    }
}

void Isobaths::setEdgePts(const QVector<QVector3D> &edgePts)
{
    //qDebug() << "Isobaths::setEdgePts" << edgePts.size();

    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->edgePts_ = edgePts;
        Q_EMIT changed();
    }
}


void Isobaths::setLineStepSize(float lineStepSize)
{
    //qDebug() << "Isobaths::setLineStepSize" << lineStepSize;

    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->lineStepSize_ = lineStepSize;
        Q_EMIT changed();
    }
}

Isobaths::IsobathsRenderImplementation::IsobathsRenderImplementation()
    : distToFocusPoint_(10.0f),
      lineStepSize_(3.0f)
{}

void Isobaths::IsobathsRenderImplementation::render(QOpenGLFunctions *ctx,
                                                    const QMatrix4x4 &model,
                                                    const QMatrix4x4 &view,
                                                    const QMatrix4x4 &projection,
                                                    const QMap<QString,
                                                    std::shared_ptr<QOpenGLShaderProgram>> &spMap) const
{
    // if (!m_isVisible ) {
    //     return;
    // }

    // if (!spMap.contains("isobaths")) {
    //     return;
    // }

    // auto& sp = spMap["isobaths"];
    // if (!sp->bind()) {
    //     qCritical() << "isobaths shader bind failed";
    //     return;
    // }

    // QMatrix4x4 mvp = projection * view * model;

    // sp->setUniformValue("matrix",        mvp);
    // sp->setUniformValue("depthMin",      minZ_);
    // sp->setUniformValue("levelStep",     levelStep_);
    // sp->setUniformValue("levelCount",    colorIntervalsSize_);
    // sp->setUniformValue("linePass",      false);
    // sp->setUniformValue("lineColor",     color_);

    // ctx->glActiveTexture(GL_TEXTURE0);
    // ctx->glBindTexture(GL_TEXTURE_2D, textureId_);
    // sp->setUniformValue("paletteSampler", 0);

    // int pos = sp->attributeLocation("position");
    // sp->enableAttributeArray(pos);
    // sp->setAttributeArray(pos, pts_.constData());
    // ctx->glDrawArrays(GL_TRIANGLES, 0, pts_.size());
    // sp->disableAttributeArray(pos);

    // if (!lineSegments_.isEmpty()) {
    //     sp->setUniformValue("linePass", true);
    //     sp->disableAttributeArray(pos);
    //     sp->enableAttributeArray(pos);
    //     sp->setAttributeArray(pos, lineSegments_.constData());
    //     ctx->glLineWidth(1.f);
    //     ctx->glDrawArrays(GL_LINES, 0, lineSegments_.size());
    //     sp->disableAttributeArray(pos);
    //     sp->setUniformValue("linePass", false);

    //     if (!labels_.isEmpty()) {
    //         glDisable(GL_DEPTH_TEST); // TODO: artifacts

    //         auto oldTextColor = TextRenderer::instance().getColor();
    //         TextRenderer::instance().setColor(QColor(color_.x(), color_.y(), color_.z()));

    //         // scale
    //         float sizeFromStep = lineStepSize_ * 0.2f;
    //         float sizeFromDist = distToFocusPoint_ * 0.0015f;
    //         float scale = qMin(sizeFromStep, sizeFromDist);
    //         scale = qBound(0.15f, scale, 0.3f);

    //         for (const auto& lbl : labels_) {
    //             QString text = QString::number(lbl.depth, 'f', 1);
    //             TextRenderer::instance().render3D(text,
    //                                               scale,
    //                                               lbl.pos,
    //                                               lbl.dir,
    //                                               ctx,
    //                                               mvp,
    //                                               spMap);
    //         }

    //         TextRenderer::instance().setColor(oldTextColor);
    //         glEnable(GL_DEPTH_TEST);
    //     }
    // }

    // sp->release();
}

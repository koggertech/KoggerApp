#include "isobaths_view.h"

#include "text_renderer.h"


IsobathsView::IsobathsView(QObject* parent)
    : SceneObject(new IsobathsViewRenderImplementation, parent)
{}

IsobathsView::~IsobathsView()
{}

void IsobathsView::clear()
{
    if (auto* r = RENDER_IMPL(IsobathsView); r) {
        r->lineSegments_.clear();
        r->labels_.clear();
    }

    Q_EMIT changed();
}

void IsobathsView::setCameraDistToFocusPoint(float val)
{
    if (auto* r = RENDER_IMPL(IsobathsView); r) {
        r->distToFocusPoint_ = val;
    }
}

void IsobathsView::setLabels(const QVector<IsobathUtils::LabelParameters> &labels)
{
    //qDebug() << "IsobathsView::setLabels" << labels.size();

    if (auto* r = RENDER_IMPL(IsobathsView); r) {
        r->labels_ = labels;
        Q_EMIT changed();
    }
}

void IsobathsView::setLineSegments(const QVector<QVector3D> &lineSegments)
{
    //qDebug() << "IsobathsView::setLineSegments" << lineSegments.size();

    if (auto* r = RENDER_IMPL(IsobathsView); r) {
        r->lineSegments_ = lineSegments;
        Q_EMIT changed();
    }
}

void IsobathsView::setLineStepSize(float lineStepSize)
{
    //qDebug() << "IsobathsView::setLineStepSize" << lineStepSize;

    if (auto* r = RENDER_IMPL(IsobathsView); r) {
        r->lineStepSize_ = lineStepSize;
        Q_EMIT changed();
    }
}

IsobathsView::IsobathsViewRenderImplementation::IsobathsViewRenderImplementation()
    : color_(1.f, 1.f, 1.f),
    distToFocusPoint_(10.0f),
    lineStepSize_(3.0f)
{}

void IsobathsView::IsobathsViewRenderImplementation::render(QOpenGLFunctions *ctx,
                                                            const QMatrix4x4 &model,
                                                            const QMatrix4x4 &view,
                                                            const QMatrix4x4 &projection,
                                                            const QMap<QString,
                                                            std::shared_ptr<QOpenGLShaderProgram>> &spMap) const
{
    if (mVis_ || !m_isVisible || lineSegments_.isEmpty()) {
        return;
    }

    auto spIt = spMap.find("isobaths");
    if (spIt == spMap.end() || !spIt.value()->bind()) {
        qWarning() << "isobaths shader not found / bind failed";
        return;
    }
    auto& sp = *spIt.value();

    const QMatrix4x4 mvp = projection * view * model;
    sp.setUniformValue("matrix",    mvp);
    sp.setUniformValue("linePass",  true);
    sp.setUniformValue("lineColor", color_);

    const int posLoc = sp.attributeLocation("position");
    sp.enableAttributeArray(posLoc);
    sp.setAttributeArray(posLoc, lineSegments_.constData());

    ctx->glLineWidth(1.f);
    ctx->glDrawArrays(GL_LINES, 0, lineSegments_.size());

    sp.disableAttributeArray(posLoc);
    sp.release();

    if (!labels_.isEmpty()) {
        glDisable(GL_DEPTH_TEST);
        const QColor oldCol = TextRenderer::instance().getColor();
        TextRenderer::instance().setColor(QColor::fromRgbF(color_.x(), color_.y(), color_.z()));

        const float sizeFromStep = lineStepSize_      * 0.20f;
        const float sizeFromDist = distToFocusPoint_  * 0.0015f;
        const float scale = qBound(0.15f, qMin(sizeFromStep, sizeFromDist), 0.30f);

        for (const auto& lbl : labels_) {
            const QString txt = QString::number(lbl.depth, 'f', 1);
            TextRenderer::instance().render3D(txt, scale, lbl.pos, lbl.dir, ctx, mvp, spMap);
        }

        TextRenderer::instance().setColor(oldCol);
        glEnable(GL_DEPTH_TEST);
    }
}

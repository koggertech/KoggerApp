#include "surface_view.h"


SurfaceView::SurfaceView(QObject* parent)
    : SceneObject(new SurfaceViewRenderImplementation, parent)
{}

SurfaceView::~SurfaceView()
{}

void SurfaceView::setBottomTrackPtr(BottomTrack* ptr)
{
    bottomTrackPtr_ = ptr;
}

void SurfaceView::onUpdatedBottomTrackData(const QVector<int>& indxs)
{
    if (!bottomTrackPtr_) {
        return;
    }

    auto* r = RENDER_IMPL(SurfaceView);
    const auto& cDataBottomTr = bottomTrackPtr_->cdata();

    for (auto itm : indxs) {
        if (r->m_data.size() <= itm) {
            r->m_data.resize(itm + 1);
        }

        r->m_data[itm] = cDataBottomTr[itm];
    }
}

SurfaceView::SurfaceViewRenderImplementation::SurfaceViewRenderImplementation()
{}

void SurfaceView::SurfaceViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &model, const QMatrix4x4 &view, const QMatrix4x4 &projection, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &spMap) const
{
    if (!m_isVisible || m_data.isEmpty()) {
        return;
    }

    if (!spMap.contains("height") || !spMap.contains("static")) {
        return;
    }

    QOpenGLShaderProgram* shProg = nullptr;

    shProg = spMap["static"].get();
    shProg->bind();

    int colorLoc  = shProg->uniformLocation("color");
    int matrixLoc = shProg->uniformLocation("matrix");
    int posLoc    = shProg->attributeLocation("position");
    int widthLoc  = shProg->uniformLocation("width");

    QVector4D vertexColor(0.91f, 0.25f, 0.2f, 1.0f);

    shProg->setUniformValue(colorLoc, vertexColor);
    shProg->setUniformValue(matrixLoc, projection * view * model);
    shProg->setUniformValue(widthLoc, 5.0f);
    shProg->enableAttributeArray(posLoc);
    shProg->setAttributeArray(posLoc, m_data.constData());

    ctx->glEnable(34370);
    ctx->glDrawArrays(GL_POINTS, 0, m_data.size());
    ctx->glDisable(34370);

    shProg->disableAttributeArray(posLoc);
    shProg->release();
}

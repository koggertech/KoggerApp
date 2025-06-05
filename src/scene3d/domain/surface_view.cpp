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
    if (!bottomTrackPtr_ || indxs.empty()) {
        return;
    }

    auto* r = RENDER_IMPL(SurfaceView);
    const auto& bTrDataRef = bottomTrackPtr_->cdata();

    for (auto itm : indxs) {

        //if (r->m_data.size() <= itm) {
        //    r->m_data.resize(itm + 1);
        //}
        //r->m_data[itm] = bTrDataRef[itm]; // ?

        // triang
        auto& point = bTrDataRef[itm];
        del_.addPoint(delaunay::Point(point.x(),point.y(), point.z()));
    }

    auto& pt = del_.getPoints();

    // треуг
    r->pts_.clear();
    for (const auto& t : del_.getTriangles()) {
        if (t.a < 4 || t.b < 4 || t.c < 4) {
            continue;
        }

        r->minZ_ = std::min(static_cast<double>(r->minZ_), pt[t.a].z);
        r->minZ_ = std::min(static_cast<double>(r->minZ_), pt[t.b].z);
        r->minZ_ = std::min(static_cast<double>(r->minZ_), pt[t.c].z);
        r->maxZ_ = std::max(static_cast<double>(r->maxZ_), pt[t.a].z);
        r->maxZ_ = std::max(static_cast<double>(r->maxZ_), pt[t.b].z);
        r->maxZ_ = std::max(static_cast<double>(r->maxZ_), pt[t.c].z);

        r->pts_.append(QVector3D(pt[t.a].x, pt[t.a].y, pt[t.a].z));
        r->pts_.append(QVector3D(pt[t.b].x, pt[t.b].y, pt[t.b].z));
        r->pts_.append(QVector3D(pt[t.c].x, pt[t.c].y, pt[t.c].z));
    }

    // ребра
    r->edgePts_.clear();
    for (int i = 0; i + 2 < r->pts_.size(); i += 3) {
        const QVector3D& a = r->pts_[i];
        const QVector3D& b = r->pts_[i + 1];
        const QVector3D& c = r->pts_[i + 2];

        r->edgePts_ << a << b;
        r->edgePts_ << b << c;
        r->edgePts_ << c << a;
    }
}

SurfaceView::SurfaceViewRenderImplementation::SurfaceViewRenderImplementation()
{}

void SurfaceView::SurfaceViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &model, const QMatrix4x4 &view, const QMatrix4x4 &projection, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &spMap) const
{
    if (!m_isVisible || pts_.empty() || edgePts_.empty() || !spMap.contains("height")) {
        return;
    }

    auto shaderProgram = spMap["height"];
    if (!shaderProgram->bind()) {
        qCritical() << "Error binding height shader program.";
        return;
    }

    int posLoc    = shaderProgram->attributeLocation("position");
    int maxZLoc   = shaderProgram->uniformLocation("max_z");
    int minZLoc   = shaderProgram->uniformLocation("min_z");
    int matrixLoc = shaderProgram->uniformLocation("matrix");

    shaderProgram->setUniformValue(minZLoc, minZ_);
    shaderProgram->setUniformValue(maxZLoc, maxZ_);
    shaderProgram->setUniformValue(matrixLoc, projection * view * model);

    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, pts_.constData());

    ctx->glEnable(GL_DEPTH_TEST);
    ctx->glDrawArrays(GL_TRIANGLES, 0, pts_.size());
    ctx->glDisable(GL_DEPTH_TEST);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();


    // ребра
    if (!edgePts_.isEmpty() && spMap.contains("static")) {
        auto lineShader = spMap["static"];
        if (lineShader->bind()) {
            int linePosLoc  = lineShader->attributeLocation("position");
            int colorLoc    = lineShader->uniformLocation("color");
            int matrixLoc   = lineShader->uniformLocation("matrix");
            int widthLoc    = lineShader->uniformLocation("width");

            lineShader->setUniformValue(matrixLoc, projection * view * model);
            lineShader->setUniformValue(colorLoc, QVector4D(0, 0, 0, 1));
            lineShader->setUniformValue(widthLoc, 1.0f);

            lineShader->enableAttributeArray(linePosLoc);
            lineShader->setAttributeArray(linePosLoc, edgePts_.constData());

            ctx->glLineWidth(1.0f);
            ctx->glDrawArrays(GL_LINES, 0, edgePts_.size());
            lineShader->disableAttributeArray(linePosLoc);
            lineShader->release();
        }
    }
}

#include "surface_view.h"

#include <QtMath>


static const QVector<QVector3D>& colorPalette(int themeId)
{
    static const QVector<QVector<QVector3D>> palettes = {
        // 0: midnight
        {
            QVector3D(0.2f, 0.5f, 1.0f),
            QVector3D(0.2f, 0.4f, 0.9f),
            QVector3D(0.3f, 0.3f, 0.8f),
            QVector3D(0.4f, 0.2f, 0.7f),
            QVector3D(0.5f, 0.2f, 0.6f),
            QVector3D(0.6f, 0.3f, 0.5f),
            QVector3D(0.7f, 0.4f, 0.4f),
            QVector3D(0.8f, 0.5f, 0.3f),
            QVector3D(0.9f, 0.6f, 0.2f),
            QVector3D(1.0f, 0.7f, 0.1f)
        },
        // 1: default
        {
            QVector3D(0.0f, 0.0f, 0.3f),
            QVector3D(0.0f, 0.0f, 0.6f),
            QVector3D(0.0f, 0.5f, 1.0f),
            QVector3D(0.0f, 1.0f, 0.5f),
            QVector3D(1.0f, 1.0f, 0.0f),
            QVector3D(1.0f, 0.6f, 0.0f),
            QVector3D(0.8f, 0.2f, 0.0f)
        },
        // 2: blue
        {
            QVector3D(0.0f, 0.0f, 0.0f),
            QVector3D(0.078f, 0.020f, 0.314f),
            QVector3D(0.196f, 0.706f, 0.902f),
            QVector3D(0.745f, 0.941f, 0.980f),
            QVector3D(1.0f, 1.0f, 1.0f)
        },
        // 3: sepia
        {
            QVector3D(0.0f, 0.0f, 0.0f),
            QVector3D(50/255.0f, 50/255.0f, 10/255.0f),
            QVector3D(230/255.0f, 200/255.0f, 100/255.0f),
            QVector3D(255/255.0f, 255/255.0f, 220/255.0f)
        },
        // 4: colored
        {
            QVector3D(0.0f, 0.0f, 0.0f),
            QVector3D(40/255.0f, 0.0f, 80/255.0f),
            QVector3D(0.0f, 30/255.0f, 150/255.0f),
            QVector3D(20/255.0f, 230/255.0f, 30/255.0f),
            QVector3D(255/255.0f, 50/255.0f, 20/255.0f),
            QVector3D(255/255.0f, 255/255.0f, 255/255.0f)
        },
        // 5: bw
        {
            QVector3D(0.0f, 0.0f, 0.0f),
            QVector3D(190/255.0f, 200/255.0f, 200/255.0f),
            QVector3D(230/255.0f, 255/255.0f, 255/255.0f)
        },
        // 6: wb
        {
            QVector3D(230/255.0f, 255/255.0f, 255/255.0f),
            QVector3D(70/255.0f, 70/255.0f, 70/255.0f),
            QVector3D(0.0f, 0.0f, 0.0f)
        }
    };

    return palettes[std::clamp(themeId, 0, palettes.size() - 1)];
}


SurfaceView::SurfaceView(QObject* parent)
    : SceneObject(new SurfaceViewRenderImplementation, parent)
{}

SurfaceView::~SurfaceView()
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        toDeleteId_ = r->textureId_;
    }
}

void SurfaceView::clear()
{
    auto* r = RENDER_IMPL(SurfaceView);

    r->pts_.clear();
    r->edgePts_.clear();
    r->minZ_ = std::numeric_limits<float>::max();
    r->maxZ_ = std::numeric_limits<float>::lowest();

    bTrToTrIndxs_.clear();

    resetTriangulation();
}

void SurfaceView::setBottomTrackPtr(BottomTrack* ptr)
{
    bottomTrackPtr_ = ptr;
}

QVector<uint8_t> &SurfaceView::getTextureTasksRef()
{
    return textureTask_;
}

GLuint SurfaceView::getDeinitTextureTask() const
{
    return toDeleteId_;
}

GLuint SurfaceView::getTextureId() const
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        return r->textureId_;
    }

    return 0;
}

void SurfaceView::setTextureId(GLuint textureId)
{
    textureId_ = textureId;

    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        r->textureId_ = textureId;
    }

    Q_EMIT changed();
}

void SurfaceView::setColorTableThemeById(int id)
{
    if (themeId_ == id) {
        return;
    }

    themeId_ = id;

    rebuildColorIntervals();

    Q_EMIT changed();
}

float SurfaceView::getSurfaceStepSize() const
{
    return surfaceStepSize_;
}

void SurfaceView::setSurfaceStepSize(float val)
{
    if (qFuzzyCompare(surfaceStepSize_, val)) {
        return;
    }

    surfaceStepSize_ = val;

    rebuildColorIntervals();

    Q_EMIT changed();
}

float SurfaceView::getLineStepSize() const
{
    return lineStepSize_;
}

void SurfaceView::setLineStepSize(float val)
{
    if (qFuzzyCompare(lineStepSize_, val)) {
        return;
    }

    lineStepSize_ = val;

    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        r->lineStepSize_ = lineStepSize_;
    }

    Q_EMIT changed();
}

void SurfaceView::onUpdatedBottomTrackData(const QVector<int>& indxs)
{
    if (!bottomTrackPtr_ || indxs.empty()) {
        return;
    }

    auto* r = RENDER_IMPL(SurfaceView);
    const auto& bTrDataRef = bottomTrackPtr_->cdata();

    auto dist2 = [](const QPointF&a,const QPointF&b) {
        double dx = a.x()-b.x();
        double dy = a.y()-b.y();
        return dx * dx + dy * dy;
    };

    for (auto itm : indxs) {
        //if (r->m_data.size() <= itm) {
        //    r->m_data.resize(itm + 1);
        //}
        //r->m_data[itm] = bTrDataRef[itm]; // ?

        auto& point = bTrDataRef[itm];

        if (bTrToTrIndxs_.contains(itm)) {
            uint64_t trIndx = bTrToTrIndxs_[itm];
            del_.getPointsRef()[trIndx].z = point.z();
        }
        else {
            if (!originSet_) {
                origin_ = { point.x(), point.y() };
                originSet_ = true;
            }

            const int cellPxVal = cellPx_;
            /* индекс ячейки */
            int ix = qFloor((point.x() - origin_.x()) / cellPxVal);
            int iy = qFloor((point.y() - origin_.y()) / cellPxVal);
            QPair<int,int> cid(ix,iy);

            QPointF center(origin_.x() + (ix + 0.5) * cellPxVal, origin_.y() + (iy + 0.5) * cellPxVal);

            if (cellPoints_.contains(cid)) {
                uint64_t indxInTr = cellPoints_[cid];

                auto& pts = del_.getPoints();
                auto lastPoint = pts[indxInTr];

                bool currNearest = dist2({ point.x(), point.y() }, center) < dist2({ lastPoint.x, lastPoint.y }, center);

                if (currNearest) {
                    bTrToTrIndxs_[itm] = del_.addPoint(delaunay::Point(point.x(),point.y(), point.z()));
                    cellPoints_[cid] = bTrToTrIndxs_[itm];// point.x(), point.y() };

                    //del_.replacePoint(indxInTr, delaunay::Point(point.x(), point.y(), point.z()));
                }
            }
            else {
                bTrToTrIndxs_[itm] = del_.addPoint(delaunay::Point(point.x(),point.y(), point.z()));
                cellPoints_[cid] = bTrToTrIndxs_[itm];// point.x(), point.y() };
            }
        }
    }

    // again
    auto& pt = del_.getPoints();

    // треуг
    r->pts_.clear();

    double lastMinZ = r->minZ_;
    double lastMaxZ = r->maxZ_;

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

    if (!(qFuzzyCompare(1.0 + lastMinZ, 1.0 + r->minZ_) &&
          qFuzzyCompare(1.0 + lastMaxZ, 1.0 + r->maxZ_))) {
        rebuildColorIntervals();
    }

    Q_EMIT changed();

}

void SurfaceView::onAction()
{
    auto& pts = del_.getPoints();

    for (auto& itm : pts) {
        qDebug() << itm.x << itm.y;
    }
}

void SurfaceView::resetTriangulation()
{
    del_ = delaunay::Delaunay();
    cellPoints_.clear();
    originSet_ = false;
}

void SurfaceView::rebuildColorIntervals()
{
    auto *r = RENDER_IMPL(SurfaceView);
    int levelCount = static_cast<int>(((r->maxZ_ - r->minZ_) / surfaceStepSize_) + 1);
    if (levelCount <= 0) {
        return;
    }

    r->colorIntervals_.clear();
    QVector<QVector3D> palette = generateExpandedPalette(levelCount);
    r->colorIntervals_.reserve(levelCount);

    for (int i = 0; i < levelCount; ++i) {
        r->colorIntervals_.append({minDepth_ + i * surfaceStepSize_, palette[i]});
    }

    r->levelStep_ = surfaceStepSize_; // ???

    updateTexture();
}

QVector<QVector3D> SurfaceView::generateExpandedPalette(int totalColors) const
{
    const auto &palette = colorPalette(themeId_);
    const int paletteSize = palette.size();

    QVector<QVector3D> retVal;

    if (totalColors <= 1 || paletteSize == 0) {
        retVal.append(paletteSize > 0 ? palette.first() : QVector3D(1.0f, 1.0f, 1.0f)); // fallback: white
        return retVal;
    }

    retVal.reserve(totalColors);

    for (int i = 0; i < totalColors; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(totalColors - 1);
        float ft = t * (paletteSize - 1);
        int i0 = static_cast<int>(ft);
        int i1 = std::min(i0 + 1, paletteSize - 1);
        float l = ft - static_cast<float>(i0);
        retVal.append((1.f - l) * palette[i0] + l * palette[i1]);
    }


    return retVal;
}

void SurfaceView::updateTexture()
{
    auto* r = RENDER_IMPL(SurfaceView);
    if (!r) {
        return;
    }

    int paletteSize = r->colorIntervals_.size();

    if (paletteSize == 0) {
        return;
    }

    textureTask_.clear();

    textureTask_.resize(paletteSize * 4);
    for (int i = 0; i < paletteSize; ++i) {
        const QVector3D &c = r->colorIntervals_[i].color;
        textureTask_[i * 4 + 0] = static_cast<uint8_t>(qBound(0.f, c.x() * 255.f, 255.f));
        textureTask_[i * 4 + 1] = static_cast<uint8_t>(qBound(0.f, c.y() * 255.f, 255.f));
        textureTask_[i * 4 + 2] = static_cast<uint8_t>(qBound(0.f, c.z() * 255.f, 255.f));
        textureTask_[i * 4 + 3] = 255;
    }
}

SurfaceView::SurfaceViewRenderImplementation::SurfaceViewRenderImplementation()
{}

void SurfaceView::SurfaceViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &model, const QMatrix4x4 &view, const QMatrix4x4 &projection, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &spMap) const
{
//     if (!m_isVisible ) {
//         return;
//     }

//     if (!spMap.contains("isobaths")) {
//         return;
//     }

//     auto& sp = spMap["isobaths"];
//     if (!sp->bind()) {
//         qCritical() << "isobaths shader bind failed";
//         return;
//     }

//     QMatrix4x4 mvp = projection * view * model;

//     sp->setUniformValue("matrix",        mvp);
//     sp->setUniformValue("depthMin",      minZ_);
//     sp->setUniformValue("invDepthRange", 1.f / (maxZ_-minZ_));
//     sp->setUniformValue("levelStep",     levelStep_);
//     sp->setUniformValue("levelCount",    colorIntervals_.size());
//     sp->setUniformValue("linePass",      false);
//     sp->setUniformValue("lineColor",     color_);

//     ctx->glActiveTexture(GL_TEXTURE0);
//     ctx->glBindTexture(GL_TEXTURE_2D, textureId_);
//     sp->setUniformValue("paletteSampler", 0);

//     int pos = sp->attributeLocation("position");
//     sp->enableAttributeArray(pos);
//     sp->setAttributeArray(pos, pts_.constData());
//     ctx->glDrawArrays(GL_TRIANGLES, 0, pts_.size());
//     sp->disableAttributeArray(pos);

//     /*
//     if (!lineSegments_.isEmpty()) {
//         sp->setUniformValue("linePass", true);
//         sp->disableAttributeArray(pos);
//         sp->enableAttributeArray(pos);
//         sp->setAttributeArray(pos, lineSegments_.constData());
//         ctx->glLineWidth(1.f);
//         ctx->glDrawArrays(GL_LINES, 0, lineSegments_.size());
//         sp->disableAttributeArray(pos);
//         sp->setUniformValue("linePass", false);

//         if (!labels_.isEmpty()) {
//             glDisable(GL_DEPTH_TEST); // TODO: artifacts

//             auto oldTextColor = TextRenderer::instance().getColor();
//             TextRenderer::instance().setColor(QColor(color_.x(), color_.y(), color_.z()));

//             // scale
//             float sizeFromStep = lineStepSize_ * 0.2f;
//             float sizeFromDist = distToFocusPoint_ * 0.0015f;
//             float scale = qMin(sizeFromStep, sizeFromDist);
//             scale = qBound(0.15f, scale, 0.3f);

//             for (const auto& lbl : labels_) {
//                 QString text = QString::number(lbl.depth, 'f', 1);
//                 TextRenderer::instance().render3D(text,
//                                                   scale,
//                                                   lbl.pos,
//                                                   lbl.dir,
//                                                   ctx,
//                                                   mvp,
//                                                   spMap);
//             }

//             TextRenderer::instance().setColor(oldTextColor);
//             glEnable(GL_DEPTH_TEST);
//         }
//     }
// */
//     sp->release();



    // old render
    if (!m_isVisible || !spMap.contains("height") || !spMap.contains("static")) {
        return;
    }

    if (trianglesVisible_) {
        if (!pts_.empty()) {
            auto shaderProgram = spMap["height"];
            if (shaderProgram->bind()) {
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
            }
        }
    }

    if (edgesVisible_) {
        if (!edgePts_.isEmpty()) {
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
}

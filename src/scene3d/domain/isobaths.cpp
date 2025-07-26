#include "isobaths.h"

#include "text_renderer.h"


Isobaths::Isobaths(QObject* parent)
    : SceneObject(new IsobathsRenderImplementation, parent),
      toDeleteId_(0)
{}

Isobaths::~Isobaths()
{
    if (auto* r = RENDER_IMPL(Isobaths); r) {
        toDeleteId_ = r->textureId_;
    }
}

void Isobaths::clear()
{
    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->pts_.clear();
        r->edgePts_.clear();
        r->minZ_ = std::numeric_limits<float>::max();
        r->maxZ_ = std::numeric_limits<float>::lowest();
        r->colorIntervalsSize_ = -1;
        r->lineSegments_.clear();
        r->labels_.clear();
    }

    textureTask_.clear();

    Q_EMIT changed();
}


QVector<uint8_t> Isobaths::takeTextureTask()
{
    auto retVal = std::move(textureTask_);
    return retVal;
}

GLuint Isobaths::getDeinitTextureTask() const
{
    return toDeleteId_;
}

GLuint Isobaths::getTextureId() const
{
    if (auto* r = RENDER_IMPL(Isobaths); r) {
        return r->textureId_;
    }

    return 0;
}

void Isobaths::setTextureId(GLuint textureId)
{
    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->textureId_ = textureId;
        Q_EMIT changed();
    }
}

void Isobaths::setCameraDistToFocusPoint(float val)
{
    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->distToFocusPoint_ = val;
    }
}

void Isobaths::setDebugMode(bool state)
{
    if (auto *r = RENDER_IMPL(Isobaths); r) {
        r->debugMode_ = state;
        Q_EMIT changed();
    }
}

void Isobaths::setTrianglesVisible(bool state)
{
    if (auto *r = RENDER_IMPL(Isobaths); r) {
        r->trianglesVisible_ = state;
        Q_EMIT changed();
    }
}

void Isobaths::setEdgesVisible(bool state)
{
    if (auto *r = RENDER_IMPL(Isobaths); r) {
        r->edgesVisible_ = state;
        Q_EMIT changed();
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

void Isobaths::setMinZ(float minZ)
{
    //qDebug() << "Isobaths::setMinZ" << minZ;

    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->minZ_ = minZ;
        Q_EMIT changed();
    }
}

void Isobaths::setMaxZ(float maxZ)
{
    //qDebug() << "Isobaths::setMaxZ" << maxZ;

    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->maxZ_ = maxZ;
        Q_EMIT changed();
    }
}

void Isobaths::setLevelStep(float levelStep)
{
    //qDebug() << "Isobaths::setLevelStep" << levelStep;

    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->levelStep_ = levelStep;
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

void Isobaths::setTextureTask(const QVector<uint8_t> &textureTask)
{
    //qDebug() << "Isobaths::setTextureTask" << textureTask.size();

    textureTask_ = textureTask;

    Q_EMIT changed();
}

void Isobaths::setColorIntervalsSize(int size)
{
    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->colorIntervalsSize_ = size;
        Q_EMIT changed();
    }
}

Isobaths::IsobathsRenderImplementation::IsobathsRenderImplementation()
    : distToFocusPoint_(10.0f),
      minZ_(std::numeric_limits<float>::max()),
      maxZ_(std::numeric_limits<float>::lowest()),
      levelStep_(3.0f),
      lineStepSize_(3.0f),
      colorIntervalsSize_(-1),
      textureId_(0),
      trianglesVisible_(false),
      edgesVisible_(false),
      debugMode_(false)
{}

void Isobaths::IsobathsRenderImplementation::render(QOpenGLFunctions *ctx,
                                                    const QMatrix4x4 &model,
                                                    const QMatrix4x4 &view,
                                                    const QMatrix4x4 &projection,
                                                    const QMap<QString,
                                                    std::shared_ptr<QOpenGLShaderProgram>> &spMap) const
{
    if (!debugMode_) {
        if (!m_isVisible ) {
            return;
        }

        if (!spMap.contains("isobaths")) {
            return;
        }

        auto& sp = spMap["isobaths"];
        if (!sp->bind()) {
            qCritical() << "isobaths shader bind failed";
            return;
        }

        QMatrix4x4 mvp = projection * view * model;

        sp->setUniformValue("matrix",        mvp);
        sp->setUniformValue("depthMin",      minZ_);
        sp->setUniformValue("levelStep",     levelStep_);
        sp->setUniformValue("levelCount",    colorIntervalsSize_);
        sp->setUniformValue("linePass",      false);
        sp->setUniformValue("lineColor",     color_);

        ctx->glActiveTexture(GL_TEXTURE0);
        ctx->glBindTexture(GL_TEXTURE_2D, textureId_);
        sp->setUniformValue("paletteSampler", 0);

        int pos = sp->attributeLocation("position");
        sp->enableAttributeArray(pos);
        sp->setAttributeArray(pos, pts_.constData());
        ctx->glDrawArrays(GL_TRIANGLES, 0, pts_.size());
        sp->disableAttributeArray(pos);

        if (!lineSegments_.isEmpty()) {
            sp->setUniformValue("linePass", true);
            sp->disableAttributeArray(pos);
            sp->enableAttributeArray(pos);
            sp->setAttributeArray(pos, lineSegments_.constData());
            ctx->glLineWidth(1.f);
            ctx->glDrawArrays(GL_LINES, 0, lineSegments_.size());
            sp->disableAttributeArray(pos);
            sp->setUniformValue("linePass", false);

            if (!labels_.isEmpty()) {
                glDisable(GL_DEPTH_TEST); // TODO: artifacts

                auto oldTextColor = TextRenderer::instance().getColor();
                TextRenderer::instance().setColor(QColor(color_.x(), color_.y(), color_.z()));

                // scale
                float sizeFromStep = lineStepSize_ * 0.2f;
                float sizeFromDist = distToFocusPoint_ * 0.0015f;
                float scale = qMin(sizeFromStep, sizeFromDist);
                scale = qBound(0.15f, scale, 0.3f);

                for (const auto& lbl : labels_) {
                    QString text = QString::number(lbl.depth, 'f', 1);
                    TextRenderer::instance().render3D(text,
                                                      scale,
                                                      lbl.pos,
                                                      lbl.dir,
                                                      ctx,
                                                      mvp,
                                                      spMap);
                }

                TextRenderer::instance().setColor(oldTextColor);
                glEnable(GL_DEPTH_TEST);
            }
        }

        sp->release();
    }
    else { // debug
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

                    ctx->glDrawArrays(GL_TRIANGLES, 0, pts_.size());

                    shaderProgram->disableAttributeArray(posLoc);
                    shaderProgram->release();
                }
            }
        }

        if (edgesVisible_) {
            if (!edgePts_.isEmpty()) {
                ctx->glDisable(GL_DEPTH_TEST);

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
                ctx->glEnable(GL_DEPTH_TEST);
            }
        }
    }
}

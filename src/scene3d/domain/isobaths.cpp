#include "isobaths.h"

#include "text_renderer.h"


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

Isobaths::Isobaths(QObject* parent)
    : SceneObject(new IsobathsRenderImplementation, parent),
      minDepth_(0.0f),
      maxDepth_(0.0f),
      surfaceStepSize_(1.0f),
      lineStepSize_(1.0f),
      labelStepSize_(100.0f),
      textureId_(0),
      toDeleteId_(0),
      themeId_(0)
{}

Isobaths::~Isobaths()
{
    if (auto* r = RENDER_IMPL(Isobaths); r) {
        toDeleteId_ = r->textureId_;
    }
}

void Isobaths::setData(const QVector<QVector3D>& data, int primitiveType)
{
    SceneObject::setData(data, primitiveType);

    processingTask_.grid = data;
    rebuildColorIntervals();

    Q_EMIT changed();
}

void Isobaths::clearData()
{
    SceneObject::clearData();

    if (auto* r = RENDER_IMPL(Isobaths)) {
        r->colorIntervals_.clear();
        r->lineSegments_.clear();
    }
    Q_EMIT changed();
}

SceneObject::SceneObjectType Isobaths::type() const
{
    return SceneObjectType::Isobaths;
}

void Isobaths::setProcessingTask(const IsobathsProcessorTask& task)
{
    processingTask_ = task;
}

IsobathsProcessorTask Isobaths::getProcessingTask() const
{
    return processingTask_;
}

float Isobaths::getSurfaceStepSize() const
{
    return surfaceStepSize_;
}

void Isobaths::setSurfaceStepSize(float val)
{
    if (qFuzzyCompare(surfaceStepSize_, val)) {
        return;
    }

    surfaceStepSize_ = val;

    rebuildColorIntervals();

    Q_EMIT changed();
}

float Isobaths::getLineStepSize() const
{
    return lineStepSize_;
}

void Isobaths::setLineStepSize(float val)
{
    if (qFuzzyCompare(lineStepSize_, val)) {
        return;
    }

    lineStepSize_ = val;

    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->lineStepSize_ = lineStepSize_;
    }

    Q_EMIT changed();
}

float Isobaths::getLabelStepSize() const
{
    return labelStepSize_;
}

void Isobaths::setLabelStepSize(float val)
{
    if (qFuzzyCompare(labelStepSize_, val)) {
        return;
    }

    labelStepSize_ = val;

    Q_EMIT changed();
}

void Isobaths::setProcessorResult(const IsobathsProcessorResult& res)
{
    if (auto* r = RENDER_IMPL(Isobaths)) {
        r->lineSegments_ = res.data;
        r->labels_ = res.labels;
    }

    Q_EMIT changed();
}

const QVector<QVector3D> &Isobaths::getRawData() const
{
    return RENDER_IMPL(Isobaths)->m_data;
}

int Isobaths::getGridWidth() const
{
    return processingTask_.gridWidth;
}

int Isobaths::getGridHeight() const
{
    return processingTask_.gridHeight;
}

QVector<uint8_t>& Isobaths::getTextureTasksRef()
{
    return textureTask_;
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
    textureId_ = textureId;

    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->textureId_ = textureId;
    }

    Q_EMIT changed();
}

void Isobaths::setCameraDistToFocusPoint(float val)
{
    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->distToFocusPoint_ = val;
    }
}

void Isobaths::setColorTableThemeById(int id)
{
    if (themeId_ == id) {
        return;
    }

    themeId_ = id;

    rebuildColorIntervals();

    Q_EMIT changed();
}

void Isobaths::rebuildColorIntervals()
{
    auto *r = RENDER_IMPL(Isobaths);
    if (!r) {
        return;
    }

    if (r->m_data.isEmpty() || surfaceStepSize_ <= 0.f) {
        return;
    }

    minDepth_ = std::numeric_limits<float>::max();
    maxDepth_ = -std::numeric_limits<float>::max();

    for (auto it = r->m_data.constBegin(); it != r->m_data.constEnd(); ++it) {
        minDepth_ = std::min(minDepth_, it->z());
        maxDepth_ = std::max(maxDepth_, it->z());
    }

    if (minDepth_ > maxDepth_) {
        std::swap(minDepth_, maxDepth_);
    }

    //minDepth_ = std::floor(minDepth_ / surfaceStepSize_) * surfaceStepSize_;
    //maxDepth_ = std::ceil (maxDepth_ / surfaceStepSize_) * surfaceStepSize_;
    int levelCount = static_cast<int>(((maxDepth_ - minDepth_) / surfaceStepSize_) + 1);

    if (levelCount <= 0) {
        return;
    }

    r->colorIntervals_.clear();
    QVector<QVector3D> palette = generateExpandedPalette(levelCount);
    r->colorIntervals_.reserve(levelCount);

    for (int i = 0; i < levelCount; ++i) {
        r->colorIntervals_.append({minDepth_ + i * surfaceStepSize_, palette[i]});
    }

    r->minDepth_ = minDepth_;
    r->maxDepth_ = maxDepth_;
    r->levelStep_ = surfaceStepSize_;

    updateTexture();
    qDebug() << "rebuildColorIntervals complete";
}

QVector<QVector3D> Isobaths::generateExpandedPalette(int totalColors) const
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

void Isobaths::updateTexture()
{
    auto* r = RENDER_IMPL(Isobaths);
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

Isobaths::IsobathsRenderImplementation::IsobathsRenderImplementation()
    : minDepth_(0.0f),
      maxDepth_(0.0f),
      levelStep_(1.0f),
      lineStepSize_(1.0f),
      textureId_(0),
      color_(0.f, 0.f, 0.f)
{

}

void Isobaths::IsobathsRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &model, const QMatrix4x4 &view, const QMatrix4x4 &projection, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &spMap) const
{
    if (!m_isVisible || m_data.isEmpty()) {
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
    sp->setUniformValue("depthMin",      minDepth_);
    sp->setUniformValue("levelStep",     levelStep_);
    sp->setUniformValue("levelCount",    colorIntervals_.size());
    sp->setUniformValue("linePass",      false);
    sp->setUniformValue("lineColor",     color_);

    ctx->glActiveTexture(GL_TEXTURE0);
    ctx->glBindTexture(GL_TEXTURE_2D, textureId_);
    sp->setUniformValue("paletteSampler", 0);

    int pos = sp->attributeLocation("position");
    sp->enableAttributeArray(pos);
    sp->setAttributeArray(pos, m_data.constData());
    ctx->glDrawArrays(m_primitiveType, 0, m_data.size());
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

#include "isobaths.h"


static const QVector<QVector3D>& colorPalette()
{
    static const QVector<QVector3D> colorPalette = {
        // midnight
        // QVector3D(0.2f, 0.5f, 1.0f),
        // QVector3D(0.2f, 0.4f, 0.9f),
        // QVector3D(0.3f, 0.3f, 0.8f),
        // QVector3D(0.4f, 0.2f, 0.7f),
        // QVector3D(0.5f, 0.2f, 0.6f),
        // QVector3D(0.6f, 0.3f, 0.5f),
        // QVector3D(0.7f, 0.4f, 0.4f),
        // QVector3D(0.8f, 0.5f, 0.3f),
        // QVector3D(0.9f, 0.6f, 0.2f),
        // QVector3D(1.0f, 0.7f, 0.1f)

        // default
        QVector3D(0.0f, 0.0f, 0.3f),
        QVector3D(0.0f, 0.0f, 0.6f),
        QVector3D(0.0f, 0.5f, 1.0f),
        QVector3D(0.0f, 1.0f, 0.5f),
        QVector3D(1.0f, 1.0f, 0.0f),
        QVector3D(1.0f, 0.6f, 0.0f),
        QVector3D(0.8f, 0.2f, 0.0f)
    };

    return colorPalette;
}


Isobaths::Isobaths(QObject* parent) :
    SceneObject(new IsobathsRenderImplementation, parent),
    paletteSize_(0),
    minDepth_(0.0f),
    maxDepth_(0.0f),
    stepSize_(1.0f),
    textureId_(0),
    toDeleteId_(0)
{}

Isobaths::~Isobaths()
{
    if (auto* r = RENDER_IMPL(Isobaths); r) {
        toDeleteId_ = r->textureId_;
    }
}

void Isobaths::setProcessingTask(const IsobathsProcessorTask& task)
{
    processingTask_ = task;
}

SceneObject::SceneObjectType Isobaths::type() const
{
    return SceneObjectType::Isobaths;
}

IsobathsProcessorTask Isobaths::processingTask() const
{
    return processingTask_;
}

void Isobaths::rebuildColorIntervals()
{
    auto *r = RENDER_IMPL(Isobaths);
    if (!r) {
        return;
    }

    if (r->m_data.isEmpty() || stepSize_ <= 0.f) {
        return;
    }

    // detect min/max Z
    minDepth_ = std::numeric_limits<float>::max();
    maxDepth_ = -std::numeric_limits<float>::max();

    for (auto it = r->m_data.constBegin(); it != r->m_data.constEnd(); ++it) {
        minDepth_ = std::min(minDepth_, it->z());
        maxDepth_ = std::max(maxDepth_, it->z());
    }

    if (minDepth_ > maxDepth_) {
        std::swap(minDepth_, maxDepth_);
    }

    // align to step grid
    minDepth_ = std::floor(minDepth_ / stepSize_) * stepSize_;
    maxDepth_ = std::ceil (maxDepth_ / stepSize_) * stepSize_;
    int levelCount = static_cast<int>(((maxDepth_ - minDepth_) / stepSize_) + 1);

    if (levelCount <= 0) {
        return;
    }

    colorIntervals_.clear();
    QVector<QVector3D> palette = generateExpandedPalette(levelCount);
    colorIntervals_.reserve(levelCount);

    for (int i = 0; i < levelCount; ++i) {
        colorIntervals_.append({minDepth_ + i * stepSize_, palette[i]});
    }

    r->colorIntervals_ = colorIntervals_;
    r->minDepth_ = minDepth_;
    r->maxDepth_ = maxDepth_;
    r->levelStep_ = stepSize_;

    updateTexture();
    qDebug() << "rebuildColorIntervals complete";

    r->paletteSize_  = paletteSize_;
}

QVector<QVector3D> Isobaths::generateExpandedPalette(int totalColors) const
{
    const auto &palette = colorPalette();
    const int paletteSize = palette.size();

    QVector<QVector3D> retVal;
    retVal.reserve(totalColors);

    for (int i = 0; i < totalColors; ++i) {
        float t = static_cast<float>((i)) / static_cast<float>(totalColors - 1);
        float ft = t * (paletteSize - 1);
        int i0 = static_cast<int>(ft);
        int i1 = std::min(i0 + 1, paletteSize - 1);
        float l = ft - static_cast<float>(i0);
        retVal.append((1.f - l) * palette[i0] + l * palette[i1]);
    }
    return retVal;
}

void Isobaths::setData(const QVector<QVector3D>& data, int primitiveType)
{
    SceneObject::setData(data, primitiveType);
    rebuildColorIntervals();
    Q_EMIT changed();
}

void Isobaths::clearData()
{
    SceneObject::clearData();
    colorIntervals_.clear();

    Q_EMIT changed();
}

Isobaths::IsobathsRenderImplementation::IsobathsRenderImplementation() :
    minDepth_(0.0f),
    maxDepth_(0.0f),
    levelStep_(1.0f),
    paletteSize_(0),
    textureId_(0)
{

}

void Isobaths::IsobathsRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!paletteSize_ || !textureId_) {
        return;
    }

    if (!m_isVisible || m_data.isEmpty()) {
        return;
    }

    if (!shaderProgramMap.contains("isobaths")) {
        return;
    }

    auto shaderProgram = shaderProgramMap["isobaths"];
    if (!shaderProgram->bind()) {
        qCritical() << "bind failed";
        return;
    }

    shaderProgram->setUniformValue("matrix", mvp);
    shaderProgram->setUniformValue("depthMin", minDepth_);
    shaderProgram->setUniformValue("invDepthRange", 1.f / (maxDepth_ - minDepth_));
    shaderProgram->setUniformValue("levelStep", levelStep_);
    shaderProgram->setUniformValue("levelCount", paletteSize_);

    QOpenGLFunctions* glFuncs = QOpenGLContext::currentContext()->functions();
    glFuncs->glActiveTexture(GL_TEXTURE0);
    glFuncs->glBindTexture(GL_TEXTURE_2D, textureId_);
    shaderProgram->setUniformValue("paletteSampler", 0);

    int posLoc = shaderProgram->attributeLocation("position");
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray   (posLoc, m_data.constData());

    ctx->glDrawArrays(m_primitiveType, 0, m_data.size());

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

float Isobaths::getStepSize() const
{
    return stepSize_;
}

void Isobaths::setStepSize(float step)
{
    if (qFuzzyCompare(stepSize_, step)) {
        return;
    }

    stepSize_ = step;
    rebuildColorIntervals();

    Q_EMIT changed();
}

QVector<float> Isobaths::getDepthLevels() const
{
    QVector<float> levels;

    for (const auto& interval : colorIntervals_) {
        levels.append(interval.depth);
    }

    return levels;
}

QVector<uint8_t>& Isobaths::getTextureTasksRef()
{
    return textureTask_;
}

GLuint Isobaths::getDeinitTextureTask() const
{
    return toDeleteId_;
}

void Isobaths::setTextureId(GLuint textureId)
{
    textureId_ = textureId;

    if (auto* r = RENDER_IMPL(Isobaths); r) {
        r->textureId_ = textureId;
    }

    Q_EMIT changed();
}

GLuint Isobaths::getTextureId() const
{
    if (auto* r = RENDER_IMPL(Isobaths); r) {
        return r->textureId_;
    }

    return 0;
}

void Isobaths::updateTexture()
{
    paletteSize_ = colorIntervals_.size();

    if (paletteSize_ == 0) {
        return;
    }

    textureTask_.clear();

    textureTask_.resize(paletteSize_ * 4);
    for (int i = 0; i < paletteSize_; ++i) {
        const QVector3D &c = colorIntervals_[i].color;
        textureTask_[i*4+0] = static_cast<uint8_t>(qBound(0.f, c.x() * 255.f, 255.f));
        textureTask_[i*4+1] = static_cast<uint8_t>(qBound(0.f, c.y() * 255.f, 255.f));
        textureTask_[i*4+2] = static_cast<uint8_t>(qBound(0.f, c.z() * 255.f, 255.f));
        textureTask_[i*4+3] = 255;
    }
}

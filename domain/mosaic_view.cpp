#include "mosaic_view.h"
#include <boundarydetector.h>
#include <Triangle.h>
#include <drawutils.h>

MosaicView::MosaicView(QObject* parent) :
    SceneObject(new MosaicViewRenderImplementation, parent),
    gen(rd()),
    dis(0.0f,1.0f),
    m_grid(std::make_shared <SurfaceGrid>())

{
    RENDER_IMPL(MosaicView)->setGens(&gen, &dis);

    QObject::connect(m_grid.get(), &SurfaceGrid::changed, [this](){
        RENDER_IMPL(MosaicView)->m_gridRenderImpl = *m_grid->m_renderImpl;
        Q_EMIT changed();
    });
}

MosaicView::~MosaicView()
{

}

SceneObject::SceneObjectType MosaicView::type() const
{
    return SceneObjectType::MosaicView;
}

void MosaicView::setData(const QVector<QVector3D>& data, int primitiveType)
{
    SceneObject::setData(data, primitiveType);

    Q_EMIT changed();
}

void MosaicView::clearData()
{
    SceneObject::clearData();
}

void MosaicView::MosaicViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible)
        return;

    auto shaderProgram = shaderProgramMap.value("mosaic", nullptr);
    if (!shaderProgram) {
        qWarning() << "Shader program 'mosaic' not found!";
        return;
    }

    shaderProgram->bind();

    shaderProgram->setUniformValue("mvp", mvp);
    int posLoc = shaderProgram->attributeLocation("position");
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, vertices_.constData());

    ctx->glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_INT, indices_.constData());

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

void MosaicView::MosaicViewRenderImplementation::setTexture(const QImage& texture)
{
    qDebug() << "setTexture";

    textureImage_ = texture;

    initializeTexture();
}

void MosaicView::MosaicViewRenderImplementation::generateRandomVertices(int width, int height, float gridSize)
{
    auto perlin = [](float x, float y) {
        return (sin(x) + cos(y)) / 2.0f;
    };

    auto perlinNoise = [perlin](float x, float y, int octaves, float scale, float persistence, float lacunarity) {
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float noiseHeight = 0.0f;

        for (int i = 0; i < octaves; ++i) {
            float sampleX = x * frequency * scale;
            float sampleY = y * frequency * scale;

            float perlinValue = perlin(sampleX, sampleY);
            noiseHeight += perlinValue * amplitude;

            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return noiseHeight;
    };

    vertices_.clear();
    indices_.clear();

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            float x = j * gridSize;
            float y = i * gridSize;
            float z = perlinNoise(x, y, 6, 0.1f, 0.5f, 2.0f) * 30.0f + ((*dis)(*gen));
            vertices_.append(QVector3D(x, y, z));
        }
    }

    for (int i = 0; i < height - 1; ++i) {
        for (int j = 0; j < width - 1; ++j) {
            int topLeft = i * width + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * width + j;
            int bottomRight = bottomLeft + 1;

            indices_.append(topLeft);
            indices_.append(bottomLeft);
            indices_.append(topRight);

            indices_.append(topRight);
            indices_.append(bottomLeft);
            indices_.append(bottomRight);
        }
    }
}

void MosaicView::MosaicViewRenderImplementation::initializeTexture()
{
    qDebug() << "initializeTexture";

    QOpenGLContext *currentContext = QOpenGLContext::currentContext();

    if (!currentContext) {
        qWarning() << "cannot initialize texture without a valid OpenGL context.";
        return;
    }

    if (textureImage_.isNull()) {
        return;
    }

    if (texture_) {
        delete texture_;
    }

    texture_ = new QOpenGLTexture(textureImage_.mirrored());
    texture_->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    texture_->setMagnificationFilter(QOpenGLTexture::Linear);
    texture_->setWrapMode(QOpenGLTexture::Repeat);

    textureInitialized_ = true;
}

void MosaicView::updateData()
{
    m_grid->clearData();

    generateRandomVertices(100, 100, 1.0f);

    // grid

    emit changed();
}

void MosaicView::setTexture(const QImage& texture)
{
    RENDER_IMPL(MosaicView)->setTexture(texture);
}

void MosaicView::generateRandomVertices(int width, int height, float gridSize)
{
    RENDER_IMPL(MosaicView)->generateRandomVertices(width, height, gridSize);
}


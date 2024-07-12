#include "mosaic_view.h"
#include <boundarydetector.h>
#include <Triangle.h>
#include <drawutils.h>

MosaicView::MosaicView(QObject* parent) :
    SceneObject(new MosaicViewRenderImplementation, parent),
    gen_(rd_()),
    dis_(0.0f,1.0f),
    grid_(std::make_shared <SurfaceGrid>())

{
    QObject::connect(grid_.get(), &SurfaceGrid::changed, [this](){
        RENDER_IMPL(MosaicView)->m_gridRenderImpl = *grid_->m_renderImpl;
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

    updateGrid();

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

    m_gridRenderImpl.render(ctx, mvp, shaderProgramMap);


    auto shaderProgram = shaderProgramMap.value("mosaic", nullptr);
    if (!shaderProgram) {
        qWarning() << "Shader program 'mosaic' not found!";
        return;
    }

    shaderProgram->bind();

    shaderProgram->setUniformValue("mvp", mvp);
    int posLoc = shaderProgram->attributeLocation("position");
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, m_data.constData());

    ctx->glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_INT, indices_.constData());

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

void MosaicView::MosaicViewRenderImplementation::setIndices(QVector<int>& indices)
{
    indices_ = indices;
}

QVector<int>& MosaicView::MosaicViewRenderImplementation::getIndicesPtr()
{
    return indices_;
}


void MosaicView::MosaicViewRenderImplementation::setTexture(const QImage& texture)
{
    qDebug() << "setTexture";

    textureImage_ = texture;

    initializeTexture();
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
    grid_->clearData();

    generateRandomVertices(width_, height_, cellSize_);

    // grid
    updateGrid();

    emit changed();
}

void MosaicView::setTexture(const QImage& texture)
{
    RENDER_IMPL(MosaicView)->setTexture(texture);
}

void MosaicView::generateRandomVertices(int width, int height, float cellSize)
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

    QVector<QVector3D> vertices;
    QVector<int> indices;

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            float x = j * cellSize;
            float y = i * cellSize;
            float z = perlinNoise(x, y, 6, 0.1f, 0.5f, 2.0f) * 30.0f + (dis_(gen_));
            vertices.append(QVector3D(x, y, z));
        }
    }

    for (int i = 0; i < height - 1; ++i) {
        for (int j = 0; j < width - 1; ++j) {
            int topLeft = i * width + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * width + j;
            int bottomRight = bottomLeft + 1;

            indices.append(topLeft);
            indices.append(bottomLeft);
            indices.append(topRight);

            indices.append(topRight);
            indices.append(bottomLeft);
            indices.append(bottomRight);
        }
    }

    RENDER_IMPL(MosaicView)->setData(vertices);
    RENDER_IMPL(MosaicView)->setIndices(indices);
}

void MosaicView::updateGrid()
{
    grid_->clearData();
    makeQuadGrid();
}

void MosaicView::makeQuadGrid()
{
    grid_->clearData();

    auto impl = RENDER_IMPL(MosaicView);

    if (impl->cdata().size() < 4)
        return;

    QVector<QVector3D> grid;

    auto indicesPtr = impl->getIndicesPtr();
    auto verticesPtr = impl->m_data;

    for (int i = 0; i < indicesPtr.size(); i += 6){
        QVector3D A = verticesPtr[indicesPtr[i]];
        QVector3D B = verticesPtr[indicesPtr[i + 1]];
        QVector3D C = verticesPtr[indicesPtr[i + 2]];
        QVector3D D = verticesPtr[indicesPtr[i + 5]];

        A.setZ(A.z() + 0.02);
        B.setZ(B.z() + 0.02);
        C.setZ(C.z() + 0.02);
        D.setZ(D.z() + 0.02);

        grid.append({ A, B, //
                      B, D,
                      A, C,
                      C, D });
    }

    grid_->setWidth(1);
    grid_->setData(grid, GL_LINES);

    impl->m_gridRenderImpl = *grid_->m_renderImpl;
}

#include "mosaic_view.h"
#include <boundarydetector.h>
#include <Triangle.h>
#include <drawutils.h>

MosaicView::MosaicView(QObject* parent) :
    SceneObject(new MosaicViewRenderImplementation, parent),
    gen_(rd_()),
    dis_(0.0f,1.0f),
    grid_(std::make_shared<SurfaceGrid>())

{
    QObject::connect(grid_.get(), &SurfaceGrid::changed, this, [this]() { // ?
        RENDER_IMPL(MosaicView)->gridRenderImpl_ = *grid_->m_renderImpl;
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

MosaicView::MosaicViewRenderImplementation::MosaicViewRenderImplementation() :
    textureId_(0)
{

}

void MosaicView::MosaicViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible)
        return;

    if (indices_.isEmpty())
        return;

    gridRenderImpl_.render(ctx, mvp, shaderProgramMap);

    auto shaderProgram = shaderProgramMap.value("mosaic", nullptr);
    if (!shaderProgram) {
        qWarning() << "Shader program 'mosaic' not found!";
        return;
    }

    shaderProgram->bind();

    shaderProgram->setUniformValue("mvp", mvp);
    int posLoc = shaderProgram->attributeLocation("position");
    int texCoordLoc = shaderProgram->attributeLocation("texCoord");

    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->enableAttributeArray(texCoordLoc);

    shaderProgram->setAttributeArray(posLoc, m_data.constData());
    shaderProgram->setAttributeArray(texCoordLoc, texCoords_.constData());

    if (textureId_) {
        glBindTexture(GL_TEXTURE_2D, textureId_);
    }

    ctx->glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_INT, indices_.constData());

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->disableAttributeArray(texCoordLoc);

    shaderProgram->release();
}

void MosaicView::setTextureId(GLuint textureId)
{
    RENDER_IMPL(MosaicView)->textureId_= textureId;

    Q_EMIT changed();
}

void MosaicView::updateData()
{
    generateRandomVertices(width_, height_, cellSize_);
    updateGrid();

    Q_EMIT changed();
}

void MosaicView::clear()
{
    grid_->clearData();
    RENDER_IMPL(MosaicView)->gridRenderImpl_.clearData();
    RENDER_IMPL(MosaicView)->indices_.clear();
    RENDER_IMPL(MosaicView)->texCoords_.clear();
    RENDER_IMPL(MosaicView)->m_data.clear();
    RENDER_IMPL(MosaicView)->textureId_ = 0;

    Q_EMIT changed();
}

void MosaicView::generateRandomVertices(int width, int height, float cellSize)
{
    int localWidth = width + 1;
    int localHeight = height + 1;

    auto perlinNoise = [](float x, float y, int octaves, float scale, float persistence, float lacunarity) {
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float noiseHeight = 0.0f;

        for (int i = 0; i < octaves; ++i) {
            float sampleX = x * frequency * scale;
            float sampleY = y * frequency * scale;

            float perlinValue = (sin(sampleX) + cos(sampleY)) / 2.0f;
            noiseHeight += perlinValue * amplitude;

            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return noiseHeight;
    };

    QVector<QVector3D> vertices;
    QVector<QVector2D> texCoords;
    QVector<int> indices;

    for (int i = 0; i < localHeight; ++i) {
        for (int j = 0; j < localWidth; ++j) {
            float x = j * cellSize;
            float y = i * cellSize;
            float z = perlinNoise(x, y, 1, 0.1f, 0.5f, 2.0f) * 3.0f + (dis_(gen_));
            vertices.append(QVector3D(x, y, z));
            texCoords.append(QVector2D(float(j) / localWidth, float(i) / localHeight));
        }
    }

    for (int i = 0; i < localHeight - 1; ++i) {
        for (int j = 0; j < localWidth - 1; ++j) {
            int topLeft = i * localWidth + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * localWidth + j;
            int bottomRight = bottomLeft + 1;

            indices.append(topLeft);
            indices.append(bottomLeft);
            indices.append(topRight);

            indices.append(topRight);
            indices.append(bottomLeft);
            indices.append(bottomRight);
        }
    }

    RENDER_IMPL(MosaicView)->m_data = vertices;
    RENDER_IMPL(MosaicView)->texCoords_ = texCoords;
    RENDER_IMPL(MosaicView)->indices_ = indices;

    Q_EMIT changed();
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

    auto indicesPtr = impl->indices_;
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

        grid.append({ A, B,
                      B, D,
                      A, C,
                      C, D });
    }

    grid_->setColor(QColor(0, 255, 0));
    grid_->setWidth(1);
    grid_->setData(grid, GL_LINES);

    impl->gridRenderImpl_ = *grid_->m_renderImpl;
}

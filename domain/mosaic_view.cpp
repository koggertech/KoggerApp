#include "mosaic_view.h"


MosaicView::MosaicView(QObject* parent) :
    SceneObject(new MosaicViewRenderImplementation, parent),
    gen_(rd_()),
    dis_(0.0f,1.0f)
{

}

MosaicView::~MosaicView()
{

}

MosaicView::MosaicViewRenderImplementation::MosaicViewRenderImplementation() :
    textureId_(0),
    gridVisible_(false)
{

}

void MosaicView::MosaicViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible)
        return;

    if (indices_.isEmpty())
        return;

    if (gridVisible_) {
        gridRenderImpl_.render(ctx, mvp, shaderProgramMap);
    }

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
    auto renderImpl = RENDER_IMPL(MosaicView);

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

    // vertices, texCoords
    QVector<QVector3D> vertices;
    QVector<QVector2D> texCoords;
    for (int i = 0; i < height_; ++i) {
        for (int j = 0; j < width_; ++j) {
            float x = j * cellSize_;
            float y = i * cellSize_;
            float z = perlinNoise(x, y, 1, 0.1f, 0.5f, 2.0f) * 3.0f + (dis_(gen_));
            vertices.append(QVector3D(x, y, z));
            texCoords.append(QVector2D(float(j) / (width_ - 1), float(i) / (height_ - 1)));
        }
    }

    // indices
    QVector<int> indices;
    for (int i = 0; i < height_ - 1; ++i) {
        for (int j = 0; j < width_ - 1; ++j) {
            if (i == 2 && j == 2) {
                continue;
            }
            int topLeft = i * width_ + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * width_ + j;
            int bottomRight = bottomLeft + 1;
            // 1--3
            // | /
            // |/
            // 2
            indices.append(topLeft);
            indices.append(bottomLeft);
            indices.append(topRight);
            //    1
            //   /|
            //  / |
            // 2--3
            indices.append(topRight);
            indices.append(bottomLeft);
            indices.append(bottomRight);
        }
    }

    // grid
    QVector<QVector3D> grid;
    for (int i = 0; i < indices.size(); i += 6) {
        QVector3D A = vertices[indices[i]];
        QVector3D B = vertices[indices[i + 1]];
        QVector3D C = vertices[indices[i + 2]];
        QVector3D D = vertices[indices[i + 5]];
        A.setZ(A.z() + 0.02);
        B.setZ(B.z() + 0.02);
        C.setZ(C.z() + 0.02);
        D.setZ(D.z() + 0.02);
        grid.append({ A, B,
                      B, D,
                      A, C,
                      C, D });
    }

    renderImpl->m_data = vertices;
    renderImpl->texCoords_ = texCoords;
    renderImpl->indices_ = indices;
    renderImpl->gridRenderImpl_.setColor(QColor(0, 255, 0));
    renderImpl->gridRenderImpl_.setData(grid, GL_LINES);

    Q_EMIT changed();
}

void MosaicView::clear()
{
    auto renderImpl = RENDER_IMPL(MosaicView);

    renderImpl->gridRenderImpl_.clearData();
    renderImpl->indices_.clear();
    renderImpl->texCoords_.clear();
    renderImpl->m_data.clear();
    renderImpl->textureId_ = 0;

    Q_EMIT changed();
}

void MosaicView::setGridVisible(bool state)
{
    RENDER_IMPL(MosaicView)->gridVisible_ = state;

    Q_EMIT changed();
}

void MosaicView::setProcTask(const SurfaceProcessorTask &task)
{
    surfaceProcTask_ = task;
}

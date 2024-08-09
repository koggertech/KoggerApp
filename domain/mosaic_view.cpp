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

MosaicView::MosaicViewRenderImplementation::MosaicViewRenderImplementation()
{
    QOpenGLContext* currentContext = QOpenGLContext::currentContext();

    //qDebug() << "        currentContext->isValid(): " <<         currentContext->isValid();

    if (!currentContext) {
        qDebug() << "NOT CURR CONT";
    }
    else {
        auto generateTexture = [](int width, int height) -> QImage {
            qDebug() << "generateTexture";


            Q_UNUSED(width);
            Q_UNUSED(height);
            QString imagePath = "C:/Users/salty/Desktop/Lenna.png";
            QImage image;
            if (!image.load(imagePath)) {
                qWarning("Не удалось загрузить изображение!");
            }
            return image;

            /*
        QImage texture(width, height, QImage::Format_RGB32);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int r = rand() % 256;
                int g = rand() % 256;
                int b = rand() % 256;
                texture.setPixel(x, y, qRgb(r, g, b));
            }
        }
        return texture;
        */
        };


        auto textureImage = generateTexture(100, 100);

        setTextureImage(textureImage);


        initializeTexture();

    }
}

void MosaicView::MosaicViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible)
        return;

    gridRenderImpl_.render(ctx, mvp, shaderProgramMap);





/*
    qDebug() << "111 needsTextureInitialization_: " << needsTextureInitialization_ << ", textureImage_.isNull(): " << textureImage_.isNull() << ", textureInitialized_: " << textureInitialized_;

    if (needsTextureInitialization_ && !textureImage_.isNull() && !textureInitialized_) {
        qDebug() << "Initializing texture in render method.";
        const_cast<MosaicViewRenderImplementation*>(this)->initializeTexture();
    }

    qDebug() << "222 needsTextureInitialization_: " << needsTextureInitialization_ << ", textureImage_.isNull(): " << textureImage_.isNull() << ", textureInitialized_: " << textureInitialized_;
*/



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

    if (!needsTextureInit_) {
        texture_->bind();
    }

    ctx->glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_INT, indices_.constData());

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->disableAttributeArray(texCoordLoc);

    shaderProgram->release();
}

void MosaicView::MosaicViewRenderImplementation::setIndices(QVector<int>& indices)
{
    indices_ = indices;
}

void MosaicView::MosaicViewRenderImplementation::setTexCoords(const QVector<QVector2D>& texCoords)
{
    texCoords_ = texCoords;
}

void MosaicView::MosaicViewRenderImplementation::setNeedsTextureInit(bool state)
{
    needsTextureInit_ = state;
}

QVector<int>& MosaicView::MosaicViewRenderImplementation::getIndicesPtr()
{
    return indices_;
}

QOpenGLTexture* MosaicView::MosaicViewRenderImplementation::getTexturePtr()
{
    return texture_;
}

QImage MosaicView::MosaicViewRenderImplementation::getTextureImagePtr()
{
    return textureImage_;
}

bool MosaicView::MosaicViewRenderImplementation::getNeedsTextureInit()
{
    return needsTextureInit_;
}

void MosaicView::MosaicViewRenderImplementation::setTexture(QOpenGLTexture* texturePtr)
{
    texture_ = texturePtr;
}

void MosaicView::MosaicViewRenderImplementation::setTextureImage(QImage texture)
{
    textureImage_ = texture;
    needsTextureInit_ = true;
}

void MosaicView::MosaicViewRenderImplementation::initializeTexture()
{
    qDebug() << "MosaicView::MosaicViewRenderImplementation::initializeTexture()";

    QOpenGLContext* currentContext = QOpenGLContext::currentContext();

    if (!currentContext) {
        qWarning() << "Cannot initialize texture without a valid OpenGL context.";
        return;
    }

    if (textureImage_.isNull()) {
        qWarning() << "Texture image is null.";
        return;
    }

    if (texture_) {
        delete texture_;
    }

    texture_ = new QOpenGLTexture(textureImage_.mirrored());
    if (texture_->isCreated()) {
        qDebug() << "Texture successfully created.";
        texture_->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        texture_->setMagnificationFilter(QOpenGLTexture::Linear);
        texture_->setWrapMode(QOpenGLTexture::Repeat);

        needsTextureInit_ = false;
    }
    else {
        qWarning() << "Failed to create texture.";
        needsTextureInit_ = true;
    }
}

void MosaicView::updateData()
{
    grid_->clearData();

    generateRandomVertices(width_, height_, cellSize_);
    RENDER_IMPL(MosaicView)->setTextureImage(generateImage(100, 100));

    updateGrid();
    emit changed();
}


void MosaicView::generateRandomVertices(int width, int height, float cellSize)
{
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

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            float x = j * cellSize;
            float y = i * cellSize;
            float z = perlinNoise(x, y, 6, 0.1f, 0.5f, 2.0f) * 30.0f + (dis_(gen_));
            vertices.append(QVector3D(x, y, z));
            texCoords.append(QVector2D(float(j) / width, float(i) / height));
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
    RENDER_IMPL(MosaicView)->setTexCoords(texCoords);
    RENDER_IMPL(MosaicView)->setIndices(indices);
}


QImage MosaicView::generateImage(int width, int height)
{
    qDebug() << "generateImage";

/*
         Q_UNUSED(width);
         Q_UNUSED(height);
         QString imagePath = "C:/Users/salty/Desktop/Lenna.png";
         QImage image;
         if (!image.load(imagePath)) {
             qWarning("Не удалось загрузить изображение!");
         }
         return image;
*/

    QImage texture(width, height, QImage::Format_RGB32);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int r = rand() % 256;
            int g = rand() % 256;
            int b = rand() % 256;
            texture.setPixel(x, y, qRgb(r, g, b));
        }
    }

    return texture;
};

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

    grid_->setColor(QColor(0, 255, 0));
    grid_->setWidth(1);
    grid_->setData(grid, GL_LINES);

    impl->gridRenderImpl_ = *grid_->m_renderImpl;
}

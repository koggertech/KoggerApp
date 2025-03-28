#include "image_view.h"

#include "scene3d_view.h"


ImageView::ImageView(QObject* parent) :
    SceneObject(new ImageViewRenderImplementation, parent),
    useLinearFilter_(false)
{ }

ImageView::~ImageView()
{ }


void ImageView::clear()
{
    // auto renderImpl = RENDER_IMPL(ImageView);

    // clear texture


    // Q_EMIT changed();
    // Q_EMIT boundsChanged();
}

void ImageView::setView(GraphicsScene3dView *viewPtr)
{
    SceneObject::m_view = viewPtr;
}

void ImageView::setTextureId(GLuint textureId)
{
    // to tile in storage and render
    // storage
    textureId_ = textureId;

    // render
    auto renderImpl = RENDER_IMPL(ImageView);
    renderImpl->textureId_ = textureId;

    Q_EMIT changed();
}

void ImageView::setUseLinearFilter(bool state)
{
    useLinearFilter_ = state;
}

GLuint ImageView::getTextureId() const
{
    // from render
    auto renderIml = RENDER_IMPL(ImageView);

    return renderIml->textureId_;
}

bool ImageView::getUseLinearFilter() const
{
    return useLinearFilter_;
}

QImage& ImageView::getTextureTasksRef()
{
    return textureTask_;
}

void ImageView::updateTexture(const QString& imagePath, QVector3D lt, QVector3D rb)
{
    //qDebug() << "ImageView::updateTexture:" << imagePath << lt << rb;
    if (imagePath.isEmpty() || lt == rb) {
        return;
    }
    if (!textureTask_.load(imagePath)) {
        return;
    }

    QTransform trans;
    trans.rotate(-90.0);
    textureTask_ = textureTask_.transformed(trans);
    lt_ = lt;
    rb_ = rb;

    QVector<QVector3D> vertices = {
        lt_,
        {rb_.x(), lt_.y(), lt_.z()},
        rb_,
        {lt_.x(), rb_.y(), lt_.z()}
    };

    QVector<QVector2D> texCoords = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    QVector<int> indices = {
        0, 1, 2,
        0, 2, 3
    };

    auto renderImpl = RENDER_IMPL(ImageView);
    renderImpl->indices_ = indices;
    renderImpl->texCoords_ = texCoords;
    renderImpl->m_data = vertices;
    renderImpl->createBounds();

    Q_EMIT boundsChanged();
    Q_EMIT changed();
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// ImageViewRenderImplementation
ImageView::ImageViewRenderImplementation::ImageViewRenderImplementation() :
    textureId_(0)
{ }

void ImageView::ImageViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp,
                                                            const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{    
    if (!m_isVisible)
        return;

    if (indices_.isEmpty() || texCoords_.isEmpty())
        return;


    auto shaderProgram = shaderProgramMap.value("image", nullptr);
    if (!shaderProgram) {
        qWarning() << "Shader program 'image' not found!";
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
        QOpenGLFunctions* glFuncs = QOpenGLContext::currentContext()->functions();
        glFuncs->glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId_);
        shaderProgram->setUniformValue("imageTexture", 0);
    }

    ctx->glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_INT, indices_.constData());

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->disableAttributeArray(texCoordLoc);

    shaderProgram->release();
}

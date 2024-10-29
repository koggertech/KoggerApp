#include "map_view.h"

#include "graphicsscene3dview.h"


MapView::MapView(GraphicsScene3dView *view, QObject *parent) :
    SceneObject(new MapViewRenderImplementation, view, parent)
{

}

MapView::~MapView()
{ }


void MapView::clear()
{
    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void MapView::setView(GraphicsScene3dView *viewPtr)
{
    SceneObject::m_view = viewPtr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// MapViewRenderImplementation
MapView::MapViewRenderImplementation::MapViewRenderImplementation()
{ }

void MapView::MapViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible)
        return;

    /*
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
    */

}

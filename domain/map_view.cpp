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

void MapView::setVec(const QVector<QVector3D> &vec)
{
    auto r = RENDER_IMPL(MapView);
    r->vec_ = vec;

    Q_EMIT changed();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// MapViewRenderImplementation
MapView::MapViewRenderImplementation::MapViewRenderImplementation()
{ }


void MapView::MapViewRenderImplementation::render(QOpenGLFunctions *ctx,
                                                      const QMatrix4x4 &model,
                                                      const QMatrix4x4 &view,
                                                      const QMatrix4x4 &projection,
                                                      const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible)
        return;

    //------------->Drawing selected vertice<<---------------//
    if (vec_.empty()) {
        return;
    }

    auto shaderProgram = shaderProgramMap["static"].get();
    shaderProgram->bind();

    auto colorLoc  = shaderProgram->uniformLocation("color");
    auto matrixLoc = shaderProgram->uniformLocation("matrix");
    auto posLoc    = shaderProgram->attributeLocation("position");

    QVector4D vertexColor(1.0f, 0.0f, 0.0f, 1.0f);

    //qDebug() << vec_;

    shaderProgram->setUniformValue(colorLoc,vertexColor);
    shaderProgram->setUniformValue(matrixLoc, projection * view * model);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, vec_.constData());


    ctx->glLineWidth(4);
    ctx->glDrawArrays(GL_LINE_LOOP, 0, vec_.size());

    shaderProgram->disableAttributeArray(posLoc);

    shaderProgram->release();
}

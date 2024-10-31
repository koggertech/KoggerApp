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

void MapView::setTiles(const QVector<map::Tile> &tiles)
{
    auto r = RENDER_IMPL(MapView);
    r->tiles_ = tiles;

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

    // ------------->Отрисовка тайлов внутри трапеции<<--------------- //
    if (!tiles_.empty()) {

        QVector<QVector3D> tileVertices;
        for (const auto& tile : tiles_) {

            float halfSize = 2.5f;
            tileVertices.append(QVector3D(tile.position.x() - halfSize, tile.position.y() - halfSize, tile.position.z()));
            tileVertices.append(QVector3D(tile.position.x() + halfSize, tile.position.y() - halfSize, tile.position.z()));
            tileVertices.append(QVector3D(tile.position.x() + halfSize, tile.position.y() + halfSize, tile.position.z()));
            tileVertices.append(QVector3D(tile.position.x() - halfSize, tile.position.y() + halfSize, tile.position.z()));
        }

        QVector4D tileColor(0.0f, 1.0f, 0.0f, 0.5f);
        shaderProgram->setUniformValue(colorLoc, tileColor);
        shaderProgram->setUniformValue(matrixLoc, projection * view * model);
        shaderProgram->enableAttributeArray(posLoc);
        shaderProgram->setAttributeArray(posLoc, tileVertices.constData());

        ctx->glLineWidth(1);
        for (int i = 0; i < tileVertices.size(); i += 4) {
            ctx->glDrawArrays(GL_LINE_LOOP, i, 4);
        }

        shaderProgram->disableAttributeArray(posLoc);
    }

    /*
    // ------------->Отрисовка заполнения с тонкими штриховыми линиями<<---------------//

    if (vec_.size() >= 4) {
        QVector<QVector3D> fillLines;

        const int numLines = 10;
        float top = vec_[0].y();
        float bottom = vec_[2].y();
        float left = vec_[0].x();
        float right = vec_[2].x();
        float step = (bottom - top) / (numLines + 1);

        for (int i = 1; i <= numLines; ++i) {
            float y = top + i * step;
            fillLines.append(QVector3D(left, y, 0.0f));
            fillLines.append(QVector3D(right, y, 0.0f));
        }

        QVector4D fillColor(0.7f, 0.4f, 0.2f, 1.0f);
        shaderProgram->setUniformValue(colorLoc, fillColor);
        shaderProgram->setUniformValue(matrixLoc, projection * view * model);
        shaderProgram->enableAttributeArray(posLoc);
        shaderProgram->setAttributeArray(posLoc, fillLines.constData());

        ctx->glLineWidth(1);
        ctx->glDrawArrays(GL_LINES, 0, fillLines.size());

        shaderProgram->disableAttributeArray(posLoc);
    }
*/

    shaderProgram->release();
}

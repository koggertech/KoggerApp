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

void MapView::setTileSetPtr(std::shared_ptr<map::TileSet> ptr)
{
    auto r = RENDER_IMPL(MapView);
    r->tileSetPtr_ = ptr;
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
    shaderProgram->release();

/*
    // ------------->Отрисовка тайлов внутри трапеции<<--------------- //
    if (!tiles_.empty()) {

        QVector<QVector3D> tileVertices;
        for (const auto& tile : tiles_) {

            float halfSize = 2.5f;
            auto vertexNed = tile.getVertexNed();
            tileVertices.append(QVector3D(vertexNed.x() - halfSize, vertexNed.y() - halfSize, vertexNed.z()));
            tileVertices.append(QVector3D(vertexNed.x() + halfSize, vertexNed.y() - halfSize, vertexNed.z()));
            tileVertices.append(QVector3D(vertexNed.x() + halfSize, vertexNed.y() + halfSize, vertexNed.z()));
            tileVertices.append(QVector3D(vertexNed.x() - halfSize, vertexNed.y() + halfSize, vertexNed.z()));
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
*/

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

    if (tileSetPtr_) {
        auto& tilesRef = tileSetPtr_->getTilesRef();

        auto shaderProgram = shaderProgramMap.value("image", nullptr);
        if (!shaderProgram) {
            qWarning() << "Shader program 'image' not found!";
            return;
        }

        shaderProgram->bind();
        shaderProgram->setUniformValue("mvp", projection * view * model);

        int posLoc = shaderProgram->attributeLocation("position");
        int texCoordLoc = shaderProgram->attributeLocation("texCoord");

        shaderProgram->enableAttributeArray(posLoc);
        shaderProgram->enableAttributeArray(texCoordLoc);

        for (auto& itm : tilesRef) {
            if (itm.second.getInUse() && itm.second.getState() == map::Tile::State::kReady) {

                auto textureId = itm.second.getTextureId();

                if (!textureId) {
                    continue;
                }

                //qDebug() << "draw" << textureId;

                auto& indices = itm.second.getIndicesRef();
                auto& vertices = itm.second.getVerticesRef();
                auto& texCoords = itm.second.getTexCoordsRef();

                shaderProgram->setAttributeArray(posLoc, vertices.constData());
                shaderProgram->setAttributeArray(texCoordLoc, texCoords.constData());

                QOpenGLFunctions* glFuncs = QOpenGLContext::currentContext()->functions();
                glFuncs->glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureId);
                shaderProgram->setUniformValue("imageTexture", 0);

                ctx->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.constData());
            }
        }

        shaderProgram->disableAttributeArray(posLoc);
        shaderProgram->disableAttributeArray(texCoordLoc);

        shaderProgram->release();
    }

}

#include "map_view.h"

#include "graphicsscene3dview.h"


MapView::MapView(GraphicsScene3dView *view, QObject *parent) :
    SceneObject(new MapViewRenderImplementation, view, parent)
{

}

MapView::~MapView()
{

}

void MapView::clear()
{
    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void MapView::setView(GraphicsScene3dView *viewPtr)
{
    SceneObject::m_view = viewPtr;
}

void MapView::setRectVertices(const QVector<QVector3D> &vertices)
{
    auto r = RENDER_IMPL(MapView);
    r->rectVertices_ = vertices;

    Q_EMIT changed();
}

void MapView::setTextureIdByTileIndx(const map::TileIndex &tileIndx, GLuint textureId)
{
    auto r = RENDER_IMPL(MapView);
    if (auto tile = r->tilesHash_.find(tileIndx); tile != r->tilesHash_.end()) {
        tile->second.setTextureId(textureId);
    }
}

std::unordered_map<map::TileIndex, QImage> MapView::getInitTileTextureTasks()
{
    QWriteLocker locker(&rWLocker_);

    auto retVal = std::move(appendTasks_);

    return retVal;
}

QList<GLuint> MapView::getDeinitTileTextureTasks()
{
    QWriteLocker locker(&rWLocker_);

    auto retVal = std::move(deleteTasks_);

    return retVal;
}

void MapView::onTileAppend(const map::Tile &tile)
{
    // append to render
    auto r = RENDER_IMPL(MapView);

    auto tileIndx = tile.getIndex();
    r->tilesHash_.emplace(tileIndx, tile);

    // append task
    appendTasks_[tileIndx] = tile.getImage();

    Q_EMIT changed();
}

void MapView::onTileDelete(const map::Tile &tile)
{
    // delete task
    auto r = RENDER_IMPL(MapView);

    if (auto tileIndx = r->tilesHash_.find(tile.getIndex()); tileIndx != r->tilesHash_.end()) {
        deleteTasks_.append(tileIndx->second.getTextureId());
    }

    // delete from render
    r->tilesHash_.erase(tile.getIndex());

    Q_EMIT changed();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// MapViewRenderImplementation
MapView::MapViewRenderImplementation::MapViewRenderImplementation()
{

}

void MapView::MapViewRenderImplementation::render(QOpenGLFunctions *ctx,
                                                      const QMatrix4x4 &model,
                                                      const QMatrix4x4 &view,
                                                      const QMatrix4x4 &projection,
                                                      const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible)
        return;

    // rect
    if (!rectVertices_.empty()) {
        auto shaderProgram = shaderProgramMap["static"].get();
        shaderProgram->bind();

        auto colorLoc  = shaderProgram->uniformLocation("color");
        auto matrixLoc = shaderProgram->uniformLocation("matrix");
        auto posLoc    = shaderProgram->attributeLocation("position");

        QVector4D vertexColor(1.0f, 0.0f, 0.0f, 1.0f);

        shaderProgram->setUniformValue(colorLoc,vertexColor);
        shaderProgram->setUniformValue(matrixLoc, projection * view * model);
        shaderProgram->enableAttributeArray(posLoc);
        shaderProgram->setAttributeArray(posLoc, rectVertices_.constData());

        ctx->glLineWidth(4);
        ctx->glDrawArrays(GL_LINE_LOOP, 0, rectVertices_.size());

        shaderProgram->disableAttributeArray(posLoc);
        shaderProgram->release();
    }

    // tiles
    if (!tilesHash_.empty()) {
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

        for (auto& itm : tilesHash_) {
            if (auto textureId = itm.second.getTextureId(); textureId) {
                const auto& indices = itm.second.getIndicesRef();
                const auto& vertices = itm.second.getVerticesRef();
                const auto& texCoords = itm.second.getTexCoordsRef();

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

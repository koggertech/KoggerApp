#include "map_view.h"


MapView::MapView(QObject *parent) :
    SceneObject(new MapViewRenderImplementation, parent)
{
    qRegisterMetaType<map::TileIndex>("map::TileIndex");
    qRegisterMetaType<GLuint>("GLuint");
}

MapView::~MapView()
{

}

void MapView::clear()
{
    auto r = RENDER_IMPL(MapView);

    // append tasks
    appendTasks_.clear();
    // delete tasks
    for (const auto& [tileIndx, tile] : r->tilesHash_) {
        deleteTasks_.append(tile.getTextureId());
    }
    // renderHash
    r->tilesHash_.clear();
    r->firstVertices_.clear();
    r->secondVertices_.clear();

    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void MapView::update()
{
    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void MapView::setRectVertices(const QVector<LLA> &firstVertices, const QVector<LLA>& secondVertices, bool green, bool isPerspective, QVector3D centralPoint)
{
    auto r = RENDER_IMPL(MapView);

    r->firstVertices_.clear();
    r->secondVertices_.clear();

    for (const auto& itm : firstVertices) {
        r->firstVertices_.append(QVector3D(itm.latitude, itm.longitude, 0.0f));
    }
    for (const auto& itm : secondVertices) {
        r->secondVertices_.append(QVector3D(itm.latitude, itm.longitude, 0.0f));
    }

    r->isGreen_ = green;
    r->isPerspective_ = isPerspective;
    r->point_ = centralPoint;

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
    auto retVal = std::move(appendTasks_);
    appendTasks_.clear();
    return retVal;
}

QList<GLuint> MapView::getDeinitTileTextureTasks()
{
    auto retVal = std::move(deleteTasks_);
    deleteTasks_.clear();
    return retVal;
}

std::unordered_map<map::TileIndex, QImage> MapView::getUpdateTileTextureTasks()
{
    auto retVal = std::move(updateImageTasks_);
    updateImageTasks_.clear();
    return retVal;
}

GLuint MapView::getTextureIdByTileIndex(const map::TileIndex& tileIndx) const
{
    // from render
    GLuint retVal = 0;
    auto r = RENDER_IMPL(MapView);
    if (auto it = r->tilesHash_.find(tileIndx); it != r->tilesHash_.end()) {
        retVal =  it->second.getTextureId();
    }

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

void MapView::onTileDelete(const map::TileIndex& tileIndx)
{
    // delete task
    auto r = RENDER_IMPL(MapView);

    if (auto it = r->tilesHash_.find(tileIndx); it != r->tilesHash_.end()) {
        deleteTasks_.append(it->second.getTextureId());
    }

    // delete from render
    r->tilesHash_.erase(tileIndx);

    Q_EMIT changed();
}

void MapView::onTileImageUpdated(const map::TileIndex& tileIndx, const QImage& image)
{
    updateImageTasks_[tileIndx] = image;
    Q_EMIT changed();
}

void MapView::onTileVerticesUpdated(const map::TileIndex& tileIndx, const QVector<QVector3D>& vertices)
{
    auto r = RENDER_IMPL(MapView);

    if (auto it = r->tilesHash_.find(tileIndx); it != r->tilesHash_.end()) {
        it->second.setVertices(vertices);
    }

    Q_EMIT changed();
}

void MapView::onClearAppendTasks()
{
    auto copyAppendTasks = std::move(appendTasks_);
    appendTasks_.clear();
    auto r = RENDER_IMPL(MapView);

    for (auto it = copyAppendTasks.begin(); it != copyAppendTasks.end(); ++it) {
        if (auto itSec = r->tilesHash_.find(it->first); itSec != r->tilesHash_.end()) {
            r->tilesHash_.erase(itSec->first);
        }

        emit deletedFromAppend(it->first);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// MapViewRenderImplementation
MapView::MapViewRenderImplementation::MapViewRenderImplementation() :
    isGreen_(false),
    isPerspective_(false),
    textureCoords_(map::kTextureCoords),
    indices_(map::kIndices)
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
/*
    // first vertices
    if (!firstVertices_.empty()) {
        auto shaderProgram = shaderProgramMap["static"].get();
        shaderProgram->bind();

        auto colorLoc  = shaderProgram->uniformLocation("color");
        auto matrixLoc = shaderProgram->uniformLocation("matrix");
        auto posLoc    = shaderProgram->attributeLocation("position");

        QVector4D vertexColor = isGreen_ ? QVector4D(0.0f, 1.0f, isPerspective_ ? 1.0f : 0.0f, 1.0f) : QVector4D(1.0f, 0.0f, isPerspective_ ? 1.0f : 0.0f, 1.0f);

        shaderProgram->setUniformValue(colorLoc,vertexColor);
        shaderProgram->setUniformValue(matrixLoc, projection * view * model);
        shaderProgram->enableAttributeArray(posLoc);
        shaderProgram->setAttributeArray(posLoc, firstVertices_.constData());

        ctx->glLineWidth(2);
        ctx->glDrawArrays(GL_LINE_LOOP, 0, firstVertices_.size());

        shaderProgram->disableAttributeArray(posLoc);
        shaderProgram->release();
    }

    // second vertices
    if (!secondVertices_.empty()) {
        auto shaderProgram = shaderProgramMap["static"].get();
        shaderProgram->bind();

        auto colorLoc  = shaderProgram->uniformLocation("color");
        auto matrixLoc = shaderProgram->uniformLocation("matrix");
        auto posLoc    = shaderProgram->attributeLocation("position");

        QVector4D vertexColor = isGreen_ ? QVector4D(0.0f, 1.0f, isPerspective_ ? 0.5f : 0.0f, 1.0f) : QVector4D(1.0f, 0.0f, isPerspective_ ? 0.5f : 0.0f, 1.0f);

        shaderProgram->setUniformValue(colorLoc,vertexColor);
        shaderProgram->setUniformValue(matrixLoc, projection * view * model);
        shaderProgram->enableAttributeArray(posLoc);
        shaderProgram->setAttributeArray(posLoc, secondVertices_.constData());

        ctx->glLineWidth(2);
        ctx->glDrawArrays(GL_LINE_LOOP, 0, secondVertices_.size());

        shaderProgram->disableAttributeArray(posLoc);
        shaderProgram->release();
    }

    // first point
    {
        auto shaderProgram = shaderProgramMap["static"].get();
        shaderProgram->bind();

        auto colorLoc  = shaderProgram->uniformLocation("color");
        auto matrixLoc = shaderProgram->uniformLocation("matrix");
        auto posLoc    = shaderProgram->attributeLocation("position");
        int widthLoc   = shaderProgram->uniformLocation("width");

        QVector4D vertexColor(0.0f, 1.0f, 0.0f, 1.0f);

        shaderProgram->setUniformValue(colorLoc,vertexColor);
        shaderProgram->setUniformValue(matrixLoc, projection * view * model);
        shaderProgram->setUniformValue(widthLoc, 8.0f);
        shaderProgram->enableAttributeArray(posLoc);
        shaderProgram->setAttributeArray(posLoc, &point_);

        ctx->glEnable(34370);
        ctx->glDrawArrays(GL_POINTS, 0, 1);
        ctx->glDisable(34370);

        shaderProgram->disableAttributeArray(posLoc);
    }

    // second point
    {
        auto shaderProgram = shaderProgramMap["static"].get();
        shaderProgram->bind();

        auto colorLoc  = shaderProgram->uniformLocation("color");
        auto matrixLoc = shaderProgram->uniformLocation("matrix");
        auto posLoc    = shaderProgram->attributeLocation("position");
        int widthLoc   = shaderProgram->uniformLocation("width");

        QVector4D vertexColor(1.f, 0.0f, 0.0f, 1.0f);

        shaderProgram->setUniformValue(colorLoc,vertexColor);
        shaderProgram->setUniformValue(matrixLoc, projection * view * model);
        shaderProgram->setUniformValue(widthLoc, 12.0f);
        shaderProgram->enableAttributeArray(posLoc);
        QVector3D point(0.0f, 0.0f, 0.0f);
        shaderProgram->setAttributeArray(posLoc, &point);

        ctx->glEnable(34370);
        ctx->glDrawArrays(GL_POINTS, 0, 1);
        ctx->glDisable(34370);
    }
*/
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

        for (auto& [tileIndx, tile] : tilesHash_) {
            if (auto textureId = tile.getTextureId(); textureId) {
                const auto& vertices = tile.getVerticesRef();

                shaderProgram->setAttributeArray(posLoc, vertices.constData());
                shaderProgram->setAttributeArray(texCoordLoc, textureCoords_.constData());

                QOpenGLFunctions* glFuncs = QOpenGLContext::currentContext()->functions();
                glFuncs->glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureId);
                shaderProgram->setUniformValue("imageTexture", 0);

                ctx->glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_INT, indices_.constData());
            }
        }

        shaderProgram->disableAttributeArray(posLoc);
        shaderProgram->disableAttributeArray(texCoordLoc);
        shaderProgram->release();
    }
}

#include "map_view.h"


static inline bool toTightRGBA8888(const QImage& in, QByteArray& out, int& w, int& h)
{
    if (in.isNull()) {
        return false;
    }

    QImage img = (in.format() == QImage::Format_RGBA8888) ? in : in.convertToFormat(QImage::Format_RGBA8888);

    w = img.width();
    h = img.height();
    if (w <= 0 || h <= 0) {
        return false;
    }

    const int dstStride = w * 4;
    const int srcStride = img.bytesPerLine();

    out.resize(dstStride * h);

    const uchar* src = img.constBits();
    uchar*       dst = reinterpret_cast<uchar*>(out.data());

    if (srcStride == dstStride) {
        memcpy(dst, src, out.size());
    }
    else {
        for (int y = 0; y < h; ++y) {
            memcpy(dst + y * dstStride, src + y * srcStride, dstStride);
        }
    }

    return true;
}


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
    appendTasks_.clear();
    updateImageTasks_.clear();

    auto r = RENDER_IMPL(MapView);
    for (const auto& [tileIndx, tile] : r->tilesHash_) {
        deleteTasks_.append(tileIndx);
    }
    r->tilesHash_.clear();

    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void MapView::update()
{
    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

std::unordered_map<map::TileIndex, QImage> MapView::takeInitTileTasks()
{
    auto tmp = std::move(appendTasks_);
    appendTasks_.clear();
    return tmp;
}

std::unordered_map<map::TileIndex, QImage> MapView::takeUpdateTileTasks()
{
    auto tmp = std::move(updateImageTasks_);
    updateImageTasks_.clear();
    return tmp;
}

QVector<map::TileIndex> MapView::takeDeleteTileTasks()
{
    auto tmp = std::move(deleteTasks_);
    deleteTasks_.clear();
    return tmp;
}

void MapView::onTileAppend(const map::Tile &tile)
{
    auto r = RENDER_IMPL(MapView);
    auto tileIndx = tile.getIndex();
    r->tilesHash_.emplace(tileIndx, tile);
    appendTasks_[tileIndx] = tile.getImage();
    Q_EMIT changed();
}

void MapView::onTileDelete(const map::TileIndex& tileIndx)
{
    deleteTasks_.append(tileIndx);

    auto r = RENDER_IMPL(MapView);
    r->tilesHash_.erase(tileIndx); // CPU-часть можно убрать сразу
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
MapView::MapViewRenderImplementation::MapViewRenderImplementation()
{}

void MapView::MapViewRenderImplementation::copyCpuSideFrom(const MapView::MapViewRenderImplementation& s)
{
    m_isVisible = s.m_isVisible;

    for (const auto& [idx, srcTile] : s.tilesHash_) { // update verts, not textId
        auto it = tilesHash_.find(idx);
        if (it == tilesHash_.end()) {
            auto t = srcTile;
            t.setTextureId(0);
            tilesHash_.emplace(idx, std::move(t));
        }
        else {
            it->second.setVertices(srcTile.getVerticesRef());
        }
    }
}

void MapView::MapViewRenderImplementation::processPendingTextureTasks(QOpenGLFunctions *gl) const
{
    if (!gl)
        return;

    gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // del
    if (!pendingDelete_.isEmpty()) {
        for (auto it = pendingDelete_.cbegin(); it != pendingDelete_.cend(); ++it) {
        //for (const auto& idx : pendingDelete_) {
            auto tIt = tilesHash_.find(*it);
            if (tIt == tilesHash_.end()) {
                continue;
            }

            GLuint tex = tIt->second.getTextureId();
            if (tex) {
                gl->glDeleteTextures(1, &tex);
                tIt->second.setTextureId(0); //
            }
            tilesHash_.erase(tIt);
        }
        pendingDelete_.clear();
    }

    // append
    if (!pendingInit_.isEmpty()) {
        for (auto& t : pendingInit_) {
            auto& idx = t.idx;

            QByteArray tight;
            int w = 0, h = 0;
            if (!toTightRGBA8888(t.img, tight, w, h)) {
                continue;
            }

            GLuint tex = 0;
            gl->glGenTextures(1, &tex);
            if (tex == 0) {
                qWarning() << "Failed to generate texture";
                continue;
            }

            gl->glBindTexture(GL_TEXTURE_2D, tex);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tight.constData());

            auto it = tilesHash_.find(idx);
            if (it != tilesHash_.end()) {
                it->second.setTextureId(tex);
            }
            else {
                map::Tile tile;
                tile.setTextureId(tex);
                tilesHash_.emplace(idx, std::move(tile));
            }
        }
        pendingInit_.clear();
    }

    // refresh
    if (!pendingUpdate_.isEmpty()) {
        for (auto& t : pendingUpdate_) {
            auto it = tilesHash_.find(t.idx);

            QByteArray tight;
            int w = 0, h = 0;
            if (!toTightRGBA8888(t.img, tight, w, h)) {
                continue;
            }

            if (it != tilesHash_.end() && it->second.getTextureId()) {
                gl->glBindTexture(GL_TEXTURE_2D, it->second.getTextureId());
                gl->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, tight.constData());
            }
            else {
                // texture does not exist
                GLuint tex = 0;
                gl->glGenTextures(1, &tex);
                if (tex == 0) {
                    qWarning() << "Failed to generate texture";
                    continue;
                }

                gl->glBindTexture(GL_TEXTURE_2D, tex);
                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tight.constData());

                if (it != tilesHash_.end()) {
                    it->second.setTextureId(tex);
                }
                else {
                    map::Tile tile; tile.setTextureId(tex);
                    tilesHash_.emplace(t.idx, std::move(tile));
                }
            }
        }
        pendingUpdate_.clear();
    }
}

void MapView::MapViewRenderImplementation::ensureQuadBuffers(QOpenGLFunctions *gl) const
{
    if (!vboUV_) {
        gl->glGenBuffers(1, &vboUV_);
        gl->glBindBuffer(GL_ARRAY_BUFFER, vboUV_);
        // фиксированный квад UV
        static const GLfloat kUV[8] = { 0,0, 1,0, 1,1, 0,1 };
        gl->glBufferData(GL_ARRAY_BUFFER, sizeof(kUV), kUV, GL_STATIC_DRAW);
        gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    if (!vboPos_) {
        gl->glGenBuffers(1, &vboPos_);
    }
}


void MapView::MapViewRenderImplementation::render(QOpenGLFunctions *ctx,
                                                      const QMatrix4x4 &model,
                                                      const QMatrix4x4 &view,
                                                      const QMatrix4x4 &projection,
                                                      const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible || !ctx) {
        return;
    }

    processPendingTextureTasks(ctx);

    if (!tilesHash_.empty()) {
        auto shaderProgram = shaderProgramMap.value("image", nullptr);
        if (!shaderProgram) {
            qWarning() << "Shader program 'image' not found!";
            return;
        }

        ensureQuadBuffers(ctx);

        shaderProgram->bind();
        shaderProgram->setUniformValue("mvp", projection * view * model);

        const int posLoc = shaderProgram->attributeLocation("position");
        const int texLoc = shaderProgram->attributeLocation("texCoord");
        if (posLoc < 0 || texLoc < 0) {
            shaderProgram->release();
            return;
        }

        ctx->glBindBuffer(GL_ARRAY_BUFFER, vboUV_);
        ctx->glVertexAttribPointer(texLoc, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const void*>(0));
        ctx->glEnableVertexAttribArray(texLoc);

        for (auto& [tileIndx, tile] : tilesHash_) {
            const GLuint tex = tile.getTextureId();
            if (!tex) {
                continue;
            }

            const auto& verts = tile.getVerticesRef();
            if (verts.size() < 4) {
                continue;
            }

            GLfloat pos[12] = {
                verts[0].x(), verts[0].y(), verts[0].z(),
                verts[1].x(), verts[1].y(), verts[1].z(),
                verts[2].x(), verts[2].y(), verts[2].z(),
                verts[3].x(), verts[3].y(), verts[3].z()
            };

            ctx->glBindBuffer(GL_ARRAY_BUFFER, vboPos_);
            ctx->glBufferData(GL_ARRAY_BUFFER, sizeof(pos), pos, GL_STREAM_DRAW);
            ctx->glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const void*>(0));
            ctx->glEnableVertexAttribArray(posLoc);

            ctx->glActiveTexture(GL_TEXTURE0);
            ctx->glBindTexture(GL_TEXTURE_2D, tex);
            shaderProgram->setUniformValue("imageTexture", 0);

            ctx->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            ctx->glDisableVertexAttribArray(posLoc);
        }

        ctx->glDisableVertexAttribArray(texLoc);
        ctx->glBindBuffer(GL_ARRAY_BUFFER, 0);
        shaderProgram->release();
    }
}

#include "surface_view.h"


SurfaceView::SurfaceView(QObject* parent)
    : SceneObject(new SurfaceViewRenderImplementation, parent),
    mosaicColorTableToDelete_(0)
{}

SurfaceView::~SurfaceView()
{
    auto* r = RENDER_IMPL(SurfaceView);
    if (!r) {
        return;
    }
    const auto& rTiles = r->tiles_;
    for (const auto& itm : rTiles) {
        mosaicTileTextureToDelete_.append(itm.getTextureId());
    }
    mosaicColorTableToDelete_ = getMosaicColorTableTextureId();
    r->mosaicColorTableTextureId_ = 0;
}

void SurfaceView::setMosaicTextureIdByTileId(QUuid tileId, GLuint textureId)
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        if (auto it = r->tiles_.find(tileId); it != r->tiles_.end()) {
            if (it.value().getTextureId() != textureId) {
                it.value().setTextureId(textureId);
                Q_EMIT changed();
            }
        }
    }
}

void SurfaceView::setMosaicColorTableTextureId(GLuint value)
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        auto& cTTId = r->mosaicColorTableTextureId_;
        if (cTTId != value) {
            cTTId = value;
            Q_EMIT changed();
        }
    }
}

GLuint SurfaceView::getMosaicTextureIdByTileId(QUuid tileId)
{
    GLuint retVal = 0;
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        if (auto it = r->tiles_.find(tileId); it != r->tiles_.end()) {
            retVal =  it.value().getTextureId();
        }
    }

    return retVal;
}

GLuint SurfaceView::getMosaicColorTableTextureId() const
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        return r->mosaicColorTableTextureId_;
    }
    return 0;
}

QVector<GLuint> SurfaceView::takeMosaicVectorTileTextureIdToDelete()
{
    auto retVal = std::move(mosaicTileTextureToDelete_);
    return retVal;
}

QVector<std::pair<QUuid, std::vector<uint8_t> > > SurfaceView::takeMosaicVectorTileTextureToAppend()
{
    auto retVal = std::move(mosaicTileTextureToAppend_);
    return retVal;
}

std::vector<uint8_t> SurfaceView::takeMosaicColorTableTextureTask()
{
    auto retVal = std::move(mosaicColorTableToAppend_);
    return retVal;
}

GLuint SurfaceView::takeMosaicColorTableDeleteTextureId()
{
    GLuint retVal = 0;
    std::swap(mosaicColorTableToDelete_, retVal);
    return retVal;
}

void SurfaceView::clear()
{
    auto* r = RENDER_IMPL(SurfaceView);
    if (!r) {
        return;
    }

    const auto& rTiles = r->tiles_;
    for (const auto& itm : rTiles) {
        mosaicTileTextureToDelete_.append(itm.getTextureId());
    }

    r->tiles_.clear();

    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void SurfaceView::setTiles(const QHash<QUuid, SurfaceTile> &tiles)
{
    //qDebug() << "SurfaceView::setTiles" << tiles.size();

    if (auto* r = RENDER_IMPL(SurfaceView); r) {

        updateMosaicTileTextureTask(tiles);

        r->tiles_ = tiles;

        Q_EMIT changed();
    }
}

void SurfaceView::setMosaicColorTableTextureTask(const std::vector<uint8_t> &colorTableTextureTask)
{
    //qDebug() << "SurfaceView::setColorTableTextureTask" << colorTableTextureTask.size();

    mosaicColorTableToAppend_ = colorTableTextureTask;
    Q_EMIT changed();
}

void SurfaceView::updateMosaicTileTextureTask(const QHash<QUuid, SurfaceTile>& newTiles) // maybe from dataProcessor
{
    //qDebug() << "SurfaceView::updateTileTextureTask" << newTiles.size();

    if (newTiles.empty()) {
        return;
    }

    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        for (auto it = r->tiles_.cbegin(); it != r->tiles_.cend(); ++it) {
            const SurfaceTile& tile = it.value();
            if (auto id = tile.getTextureId(); id) {
                mosaicTileTextureToDelete_.push_back(id);
            }
        }

        mosaicTileTextureToAppend_.clear();
        mosaicTileTextureToAppend_.reserve(newTiles.size());
        for (auto it = newTiles.cbegin(); it != newTiles.cend(); ++it) {
            const QUuid& tileUuid = it.key();
            const SurfaceTile& tile = it.value();
            mosaicTileTextureToAppend_.push_back(std::make_pair(tileUuid, tile.getImageDataCRef()));
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// SurfaceViewRenderImplementation
SurfaceView::SurfaceViewRenderImplementation::SurfaceViewRenderImplementation() :
    mosaicColorTableTextureId_(0)
{
#if defined(Q_OS_ANDROID) || defined(LINUX_ES)
    colorTableTextureType_ = GL_TEXTURE_2D;
#else
    mosaicColorTableTextureType_ = GL_TEXTURE_1D;
#endif
}

void SurfaceView::SurfaceViewRenderImplementation::render(QOpenGLFunctions *ctx,
                                                            const QMatrix4x4 &mvp,
                                                            const QMap<QString,
                                                            std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible) {
        return;
    }

    auto shaderProgram = shaderProgramMap.value("mosaic", nullptr);
    if (!shaderProgram) {
        qWarning() << "Shader program 'mosaic' not found!";
        return;
    }

    // tiles
    for (auto& itm : tiles_) {
        if (!itm.getIsInited()) {
            continue;
        }
        GLuint textureId = itm.getTextureId();
        if (!textureId || !mosaicColorTableTextureId_) {
            continue;
        }

        shaderProgram->bind();
        shaderProgram->setUniformValue("mvp", mvp);

        int positionLoc = shaderProgram->attributeLocation("position");
        int texCoordLoc = shaderProgram->attributeLocation("texCoord");

        shaderProgram->enableAttributeArray(positionLoc);
        shaderProgram->enableAttributeArray(texCoordLoc);

        shaderProgram->setAttributeArray(positionLoc , itm.getHeightVerticesConstRef().constData());
        shaderProgram->setAttributeArray(texCoordLoc, itm.getTextureVerticesRef().constData());


        QOpenGLFunctions* glFuncs = QOpenGLContext::currentContext()->functions();

        glFuncs->glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        shaderProgram->setUniformValue("indexedTexture", 0);

        glFuncs->glActiveTexture(GL_TEXTURE1);
        glBindTexture(mosaicColorTableTextureType_, mosaicColorTableTextureId_);
        shaderProgram->setUniformValue("colorTable", 1);

        ctx->glDrawElements(GL_TRIANGLES, itm.getHeightIndicesRef().size(), GL_UNSIGNED_INT, itm.getHeightIndicesRef().constData());

        shaderProgram->disableAttributeArray(texCoordLoc);
        shaderProgram->disableAttributeArray(positionLoc);

        shaderProgram->release();
    }
}

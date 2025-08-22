#include "surface_view.h"


SurfaceView::SurfaceView(QObject* parent)
    : SceneObject(new SurfaceViewRenderImplementation, parent),
    mosaicColorTableToDelete_(0),
    surfaceColorTableToDelete_(0)
{}

SurfaceView::~SurfaceView()
{
    auto* r = RENDER_IMPL(SurfaceView);
    if (!r) {
        return;
    }
    const auto& rTiles = r->tiles_;
    for (const auto& itm : rTiles) {
        mosaicTileTextureToDelete_.append(itm.getMosaicTextureId());
    }
    mosaicColorTableToDelete_ = getMosaicColorTableTextureId();
    r->mosaicColorTableTextureId_ = 0;

    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        surfaceColorTableToDelete_ = r->surfaceColorTableTextureId_;
    }
}

void SurfaceView::setMosaicTextureIdByTileId(QUuid tileId, GLuint textureId)
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        if (auto it = r->tiles_.find(tileId); it != r->tiles_.end()) {
            if (it.value().getMosaicTextureId() != textureId) {
                it.value().setMosaicTextureId(textureId);
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

GLuint SurfaceView::getMosaicTextureIdByTileId(QUuid tileId) const
{
    GLuint retVal = 0;
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        if (auto it = r->tiles_.find(tileId); it != r->tiles_.end()) {
            retVal =  it.value().getMosaicTextureId();
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

QVector<GLuint> SurfaceView::takeMosaicTileTextureToDelete()
{
    QMutexLocker lock(&mosaicTexTasksMutex_);

    std::sort(mosaicTileTextureToDelete_.begin(), mosaicTileTextureToDelete_.end());
    mosaicTileTextureToDelete_.erase(std::unique(mosaicTileTextureToDelete_.begin(), mosaicTileTextureToDelete_.end()), mosaicTileTextureToDelete_.end());

    QVector<GLuint> out;
    out.swap(mosaicTileTextureToDelete_);
    return out;
}

QVector<std::pair<QUuid, std::vector<uint8_t> > > SurfaceView::takeMosaicTileTextureToAppend()
{
    QMutexLocker lock(&mosaicTexTasksMutex_);

    QVector<std::pair<QUuid, std::vector<uint8_t>>> out;
    out.reserve(mosaicTileTextureToAppend_.size());
    for (auto it = mosaicTileTextureToAppend_.cbegin(); it != mosaicTileTextureToAppend_.cend(); ++it) {
        out.push_back({ it.key(), it.value() });
    }
    mosaicTileTextureToAppend_.clear();
    return out;
}

std::vector<uint8_t> SurfaceView::takeMosaicColorTableToAppend()
{
    auto retVal = std::move(mosaicColorTableToAppend_);
    return retVal;
}

GLuint SurfaceView::takeMosaicColorTableToDelete()
{
    GLuint retVal = 0;
    std::swap(mosaicColorTableToDelete_, retVal);
    return retVal;
}

QVector<uint8_t> SurfaceView::takeSurfaceColorTableToAppend()
{
    auto retVal = std::move(surfaceColorTableToAppend_);
    return retVal;
}

GLuint SurfaceView::takeSurfaceColorTableToDelete()
{
    GLuint retVal = 0;
    std::swap(surfaceColorTableToDelete_, retVal);
    return retVal;
}

GLuint SurfaceView::getSurfaceColorTableTextureId() const
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        return r->surfaceColorTableTextureId_;
    }

    return 0;
}

bool SurfaceView::getMVisible() const
{
    auto* r = RENDER_IMPL(SurfaceView);
    return r->mVis_;
}

bool SurfaceView::getIVisible() const
{
    auto* r = RENDER_IMPL(SurfaceView);
    return r->iVis_;
}

void SurfaceView::setSurfaceColorTableTextureId(GLuint textureId)
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        r->surfaceColorTableTextureId_ = textureId;
        Q_EMIT changed();
    }
}

void SurfaceView::setIVisible(bool state)
{
    auto* r = RENDER_IMPL(SurfaceView);
    r->iVis_ = state;
    Q_EMIT changed();
}

void SurfaceView::setMVisible(bool state)
{
    auto* r = RENDER_IMPL(SurfaceView);
    r->mVis_ = state;
    Q_EMIT changed();
}

void SurfaceView::clear()
{
    auto* r = RENDER_IMPL(SurfaceView);
    if (!r) {
        return;
    }

    const auto& rTiles = r->tiles_;
    for (const auto& itm : rTiles) {
        mosaicTileTextureToDelete_.append(itm.getMosaicTextureId());
    }

    r->tiles_.clear();

    {
        QMutexLocker lock(&mosaicTexTasksMutex_);
        mosaicTileTextureToAppend_.clear();
    }

    //r->minZ_ = std::numeric_limits<float>::max();
    //r->maxZ_ = std::numeric_limits<float>::lowest();
    //r->colorIntervalsSize_ = -1;

    surfaceColorTableToAppend_.clear();


    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void SurfaceView::setTiles(const QHash<QUuid, SurfaceTile> &tiles, bool useTextures)
{
    //qDebug() << "SurfaceView::setTiles" << tiles.size();

    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        auto& rTRef = r->tiles_;

        for (auto itT = tiles.cbegin(); itT != tiles.cend(); ++itT) {
            auto& iKey   = itT.key();
            auto& iValue = itT.value();

            if (auto itRT = rTRef.find(iKey); itRT != rTRef.end()) { //refresh
                auto& itRTVRef = itRT.value();
                const auto savedTexId = itRTVRef.textureId_;
                itRTVRef = std::move(iValue);
                itRTVRef.textureId_ = savedTexId;
            }
            else {
                rTRef.insert(iKey, iValue);
            }
        }

        if (useTextures) {
            updateMosaicTileTextureTask(tiles);
        }

        Q_EMIT changed();
    }
}

void SurfaceView::setMosaicColorTableTextureTask(const std::vector<uint8_t> &colorTableTextureTask)
{
    //qDebug() << "SurfaceView::setColorTableTextureTask" << colorTableTextureTask.size();

    mosaicColorTableToAppend_ = colorTableTextureTask;
    Q_EMIT changed();
}

void SurfaceView::setMinZ(float minZ)
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        r->minZ_ = minZ;
        Q_EMIT changed();
    }
}

void SurfaceView::setMaxZ(float maxZ)
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        r->maxZ_ = maxZ;
        Q_EMIT changed();
    }
}

void SurfaceView::setSurfaceStep(float surfaceStep)
{
    //qDebug() << "SurfaceView::setSurfaceStep" << levelStep;

    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        r->surfaceStep_ = surfaceStep;
        Q_EMIT changed();
    }
}

void SurfaceView::setTextureTask(const QVector<uint8_t> &textureTask)
{
    surfaceColorTableToAppend_ = textureTask;

    Q_EMIT changed();
}

void SurfaceView::setColorIntervalsSize(int size)
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        r->colorIntervalsSize_ = size;
        Q_EMIT changed();
    }
}

void SurfaceView::removeTiles(const QSet<QUuid> &ids)
{
    if (ids.isEmpty()) return;
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        for (const auto& id : ids) {
            auto it = r->tiles_.find(id);
            if (it != r->tiles_.end()) {
                if (auto oldId = it.value().getMosaicTextureId(); oldId != 0) {
                    mosaicTileTextureToDelete_.append(oldId);
                }
                r->tiles_.erase(it);
            }
        }
        Q_EMIT changed();
    }
}

void SurfaceView::updateMosaicTileTextureTask(const QHash<QUuid, SurfaceTile>& newTiles)
{
    if (newTiles.isEmpty()) {
        return;
    }
    auto* r = RENDER_IMPL(SurfaceView);

    if (!r) {
        return;
    }

    {
        QMutexLocker lock(&mosaicTexTasksMutex_);
        auto& rTiles = r->tiles_;

        for (auto it = newTiles.cbegin(); it != newTiles.cend(); ++it) {
            const auto& key   = it.key();
            const auto& value = it.value();

            if (auto rIt = rTiles.find(key); rIt != rTiles.end()) { // только апдейт
                auto& tile = rIt.value();
                tile.imageData_ = value.imageData_;
                mosaicTileTextureToAppend_.insert(key, tile.getMosaicImageDataCRef());
            }
            else {
                qWarning() << "SurfaceView: no tile for key" << key;
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// SurfaceViewRenderImplementation
SurfaceView::SurfaceViewRenderImplementation::SurfaceViewRenderImplementation()
    : surfaceColorTableTextureId_(0),
    mosaicColorTableTextureId_(0),
    minZ_(std::numeric_limits<float>::max()),
    maxZ_(std::numeric_limits<float>::lowest()),
    surfaceStep_(3.0f),
    colorIntervalsSize_(-1),
    iVis_(false),
    mVis_(false)
{
#if defined(Q_OS_ANDROID) || defined(LINUX_ES)
    mosaicColorTableTextureType_ = GL_TEXTURE_2D;
#else
    mosaicColorTableTextureType_ = GL_TEXTURE_1D;
#endif
}

void SurfaceView::SurfaceViewRenderImplementation::render(QOpenGLFunctions *ctx,
                                                            const QMatrix4x4 &mvp,
                                                            const QMap<QString,
                                                            std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!iVis_ && !mVis_) {
        return;
    }

    auto mShP = shaderProgramMap.value("mosaic", nullptr);
    auto iShP = shaderProgramMap.value("isobaths", nullptr);

    if (!mShP || !iShP) {
        qWarning() << "Shader program 'mosaic'|'isobaths' not found!";
        return;
    }

    // tiles TODO OPTIMIZE
    for (auto& itm : tiles_) {
        if (!itm.getIsInited()) {
            continue;
        }

        GLuint textureId = itm.getMosaicTextureId();

        if (mVis_) {
            auto& shP = mShP;

            shP->bind();
            shP->setUniformValue("mvp", mvp);

            int positionLoc = shP->attributeLocation("position");
            int texCoordLoc = shP->attributeLocation("texCoord");

            shP->enableAttributeArray(positionLoc);
            shP->enableAttributeArray(texCoordLoc);

            shP->setAttributeArray(positionLoc, itm.getHeightVerticesCRef().constData());
            shP->setAttributeArray(texCoordLoc, itm.getMosaicTextureVerticesCRef().constData());

            QOpenGLFunctions* glFuncs = QOpenGLContext::currentContext()->functions();

            glFuncs->glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureId);
            shP->setUniformValue("indexedTexture", 0);

            glFuncs->glActiveTexture(GL_TEXTURE1);
            glBindTexture(mosaicColorTableTextureType_, mosaicColorTableTextureId_);
            shP->setUniformValue("colorTable", 1);

            ctx->glDrawElements(GL_TRIANGLES,
                                itm.getHeightIndicesCRef().size(),
                                GL_UNSIGNED_INT,
                                itm.getHeightIndicesCRef().constData());

            shP->disableAttributeArray(texCoordLoc);
            shP->disableAttributeArray(positionLoc);

            shP->release();
        }
        else if (iVis_) {
            auto &shP = iShP;
            shP->bind();

            shP->setUniformValue("matrix",     mvp);
            shP->setUniformValue("depthMin",   minZ_);
            shP->setUniformValue("levelStep",  surfaceStep_);
            shP->setUniformValue("levelCount", colorIntervalsSize_);
            shP->setUniformValue("linePass",   false);   // ни линий, ни лейблов

            ctx->glActiveTexture(GL_TEXTURE0);
            ctx->glBindTexture(GL_TEXTURE_2D, surfaceColorTableTextureId_);
            shP->setUniformValue("paletteSampler", 0);

            const int posLoc = shP->attributeLocation("position");
            shP->enableAttributeArray(posLoc);
            shP->setAttributeArray(posLoc, itm.getHeightVerticesCRef().constData());

            ctx->glDrawElements(GL_TRIANGLES,
                                itm.getHeightIndicesCRef().size(),
                                GL_UNSIGNED_INT,
                                itm.getHeightIndicesCRef().constData());

            shP->disableAttributeArray(posLoc);
            shP->release();
        }
    }
}

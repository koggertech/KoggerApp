#include "surface_view.h"


SurfaceView::SurfaceView(QObject* parent)
    : SceneObject(new SurfaceViewRenderImplementation, parent),
    mosaicColorTableToDelete_(0),
    toDeleteId_(0)
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
        toDeleteId_ = r->textureId_;
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

GLuint SurfaceView::getMosaicTextureIdByTileId(QUuid tileId)
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
    auto retVal = std::move(mosaicTileTextureToDelete_);
    return retVal;
}

QVector<std::pair<QUuid, std::vector<uint8_t> > > SurfaceView::takeMosaicTileTextureToAppend()
{
    auto retVal = std::move(mosaicTileTextureToAppend_);
    return retVal;
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
    //qDebug() << "t" << textureTask_.size();
    auto retVal = std::move(textureTask_);
    return retVal;
}

GLuint SurfaceView::takeSurfaceColorTableToDelete()
{
    GLuint retVal = 0;
    std::swap(toDeleteId_, retVal);
    return retVal;
}

GLuint SurfaceView::getSurfaceColorTableTextureId() const
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        return r->textureId_;
    }

    return 0;
}

void SurfaceView::setSurfaceColorTableTextureId(GLuint textureId)
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        r->textureId_ = textureId;
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


    r->minZ_ = std::numeric_limits<float>::max();
    r->maxZ_ = std::numeric_limits<float>::lowest();
    r->colorIntervalsSize_ = -1;

    textureTask_.clear();


    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void SurfaceView::setTiles(const QHash<QUuid, SurfaceTile> &tiles, bool useTextures)
{
    //qDebug() << "SurfaceView::setTiles" << tiles.size();

    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        if (useTextures){
            updateMosaicTileTextureTask(tiles);
        }

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
    textureTask_ = textureTask;

    Q_EMIT changed();
}

void SurfaceView::setColorIntervalsSize(int size)
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        r->colorIntervalsSize_ = size;
        Q_EMIT changed();
    }
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
            if (auto id = tile.getMosaicTextureId(); id) {
                mosaicTileTextureToDelete_.push_back(id);
            }
        }

        mosaicTileTextureToAppend_.clear();
        mosaicTileTextureToAppend_.reserve(newTiles.size());
        for (auto it = newTiles.cbegin(); it != newTiles.cend(); ++it) {
            const QUuid& tileUuid = it.key();
            const SurfaceTile& tile = it.value();
            mosaicTileTextureToAppend_.push_back(std::make_pair(tileUuid, tile.getMosaicImageDataCRef()));
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// SurfaceViewRenderImplementation
SurfaceView::SurfaceViewRenderImplementation::SurfaceViewRenderImplementation() :
    mosaicColorTableTextureId_(0),
    minZ_(std::numeric_limits<float>::max()),
    maxZ_(std::numeric_limits<float>::lowest()),
    surfaceStep_(3.0f),
    colorIntervalsSize_(-1),
    textureId_(0)
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

        if (iVis_ && !mVis_) {
            auto &shP = iShP;
            shP->bind();

            shP->setUniformValue("matrix",     mvp);
            shP->setUniformValue("depthMin",   minZ_);
            shP->setUniformValue("levelStep",  surfaceStep_);
            shP->setUniformValue("levelCount", colorIntervalsSize_);
            shP->setUniformValue("linePass",   false);   // ни линий, ни лейблов

            ctx->glActiveTexture(GL_TEXTURE0);
            ctx->glBindTexture(GL_TEXTURE_2D, textureId_);
            shP->setUniformValue("paletteSampler", 0);

            const int posLoc = shP->attributeLocation("position");
            shP->enableAttributeArray(posLoc);
            shP->setAttributeArray(posLoc, itm.getHeightVerticesConstRef().constData());

            ctx->glDrawElements(GL_TRIANGLES,
                                itm.getHeightIndicesRef().size(),
                                GL_UNSIGNED_INT,
                                itm.getHeightIndicesRef().constData());

            shP->disableAttributeArray(posLoc);
            shP->release();
        }
        else if (mVis_) {
            auto& shP = mShP;

            shP->bind();
            shP->setUniformValue("mvp", mvp);

            int positionLoc = shP->attributeLocation("position");
            int texCoordLoc = shP->attributeLocation("texCoord");

            shP->enableAttributeArray(positionLoc);
            shP->enableAttributeArray(texCoordLoc);

            shP->setAttributeArray(positionLoc, itm.getHeightVerticesConstRef().constData());
            shP->setAttributeArray(texCoordLoc, itm.getMosaicTextureVerticesRef().constData());

            QOpenGLFunctions* glFuncs = QOpenGLContext::currentContext()->functions();

            glFuncs->glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureId);
            shP->setUniformValue("indexedTexture", 0);

            glFuncs->glActiveTexture(GL_TEXTURE1);
            glBindTexture(mosaicColorTableTextureType_, mosaicColorTableTextureId_);
            shP->setUniformValue("colorTable", 1);

            ctx->glDrawElements(GL_TRIANGLES,
                                itm.getHeightIndicesRef().size(),
                                GL_UNSIGNED_INT,
                                itm.getHeightIndicesRef().constData());

            shP->disableAttributeArray(texCoordLoc);
            shP->disableAttributeArray(positionLoc);

            shP->release();
        }
    }
}

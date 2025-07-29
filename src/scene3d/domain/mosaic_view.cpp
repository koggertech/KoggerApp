#include "mosaic_view.h"


MosaicView::MosaicView(QObject* parent) :
    SceneObject(new MosaicViewRenderImplementation, parent)
{
}

MosaicView::~MosaicView()
{
    auto* r = RENDER_IMPL(MosaicView);
    if (!r) {
        return;
    }
    const auto& rTiles = r->tiles_;
    for (const auto& itm : rTiles) {
        vectorTileTextureIdToDelete_.append(itm.getTextureId());
    }
    colorTableDeleteTextureId_ = getColorTableTextureId();
    r->colorTableTextureId_ = 0;
}

void MosaicView::clear()
{
    auto* r = RENDER_IMPL(MosaicView);
    if (!r) {
        return;
    }

    const auto& rTiles = r->tiles_;
    for (const auto& itm : rTiles) {
        vectorTileTextureIdToDelete_.append(itm.getTextureId());
    }

    r->tiles_.clear();
    r->measLinesVertices_.clear();
    r->measLinesEvenIndices_.clear();
    r->measLinesOddIndices_.clear();
    r->updateBounds();

    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void MosaicView::setTextureIdByTileId(QUuid tileId, GLuint textureId)
{
    if (auto* r = RENDER_IMPL(MosaicView); r) {
        if (auto it = r->tiles_.find(tileId); it != r->tiles_.end()) {
            if (it.value().getTextureId() != textureId) {
                it.value().setTextureId(textureId);
                Q_EMIT changed();
            }
        }
    }
}

void MosaicView::setColorTableTextureId(GLuint value)
{
    if (auto* r = RENDER_IMPL(MosaicView); r) {
        auto& cTTId = r->colorTableTextureId_;
        if (cTTId != value) {
            cTTId = value;
            Q_EMIT changed();
        }
    }
}

void MosaicView::setMeasLineVisible(bool state)
{
    if (auto* r = RENDER_IMPL(MosaicView); r) {
        if (r->measLineVisible_ != state) {
            r->measLineVisible_ = state;
            Q_EMIT changed();
        }
    }
}

void MosaicView::setTileGridVisible(bool state)
{
    if (auto* r = RENDER_IMPL(MosaicView); r) {
        if (r->tileGridVisible_ != state) {
            r->tileGridVisible_ = state;
            Q_EMIT changed();
        }
    }
}

void MosaicView::setUseLinearFilter(bool state)
{
    if (auto* r = RENDER_IMPL(MosaicView); r) {
        if (useLinearFilter_ != state) {
            useLinearFilter_ = state;
            updateTileTextureTask(r->tiles_);
            Q_EMIT changed();
        }
    }
}

QVector<GLuint> MosaicView::takeVectorTileTextureIdToDelete()
{
    auto retVal = std::move(vectorTileTextureIdToDelete_);
    return retVal;
}

QVector<std::pair<QUuid, std::vector<uint8_t> > > MosaicView::takeVectorTileTextureToAppend()
{
    auto retVal = std::move(vectorTileTextureToAppend_);
    return retVal;
}

std::vector<uint8_t> MosaicView::takeColorTableTextureTask()
{
    auto retVal = std::move(colorTableTextureTask_);
    return retVal;
}

GLuint MosaicView::takeColorTableDeleteTextureId()
{
    GLuint retVal = 0;
    std::swap(colorTableDeleteTextureId_, retVal);
    return retVal;
}

GLuint MosaicView::getTextureIdByTileId(QUuid tileId)
{
    GLuint retVal = 0;
    if (auto* r = RENDER_IMPL(MosaicView); r) {
        if (auto it = r->tiles_.find(tileId); it != r->tiles_.end()) {
            retVal =  it.value().getTextureId();
        }
    }

    return retVal;
}

GLuint MosaicView::getColorTableTextureId() const
{
    if (auto* r = RENDER_IMPL(MosaicView); r) {
        return r->colorTableTextureId_;
    }
    return 0;
}

bool MosaicView::getUseLinearFilter() const
{
    return useLinearFilter_;
}

void MosaicView::setTiles(const QHash<QUuid, SurfaceTile> &tiles)
{
    //qDebug() << "MosaicView::setTiles" << tiles.size();

    if (auto* r = RENDER_IMPL(MosaicView); r) {

        updateTileTextureTask(tiles);

        r->tiles_ = tiles;

        Q_EMIT changed();
    }
}

void MosaicView::setMeasLinesVertices(const QVector<QVector3D> &measLinesVertices)
{
    qDebug() << "MosaicView::setMeasLinesVertices" << measLinesVertices.size();

    if (auto* r = RENDER_IMPL(MosaicView); r) {
        r->measLinesVertices_ = measLinesVertices;
        Q_EMIT changed();
    }
}

void MosaicView::setMeasLinesEvenIndices(const QVector<int> &measLinesEvenIndices)
{
    qDebug() << "MosaicView::setMeasLinesEvenIndices" << measLinesEvenIndices.size();

    if (auto* r = RENDER_IMPL(MosaicView); r) {
        r->measLinesEvenIndices_ = measLinesEvenIndices;
        Q_EMIT changed();
    }
}

void MosaicView::setMeasLinesOddIndices(const QVector<int> &measLinesOddIndices)
{
    qDebug() << "MosaicView::setMeasLinesOddIndices" << measLinesOddIndices.size();

    if (auto* r = RENDER_IMPL(MosaicView); r) {
        r->measLinesOddIndices_ = measLinesOddIndices;
        Q_EMIT changed();
    }
}

void MosaicView::setColorTableTextureTask(const std::vector<uint8_t> &colorTableTextureTask)
{
    //qDebug() << "MosaicView::setColorTableTextureTask" << colorTableTextureTask.size();

    colorTableTextureTask_ = colorTableTextureTask;
    Q_EMIT changed();
}

void MosaicView::updateTileTextureTask(const QHash<QUuid, SurfaceTile>& newTiles) // maybe from dataProcessor
{
    //qDebug() << "MosaicView::updateTileTextureTask" << newTiles.size();

    if (newTiles.empty()) {
        return;
    }

    if (auto* r = RENDER_IMPL(MosaicView); r) {
        for (auto it = r->tiles_.cbegin(); it != r->tiles_.cend(); ++it) {
            const SurfaceTile& tile = it.value();
            if (auto id = tile.getTextureId(); id) {
                vectorTileTextureIdToDelete_.push_back(id);
            }
        }

        vectorTileTextureToAppend_.clear();
        vectorTileTextureToAppend_.reserve(newTiles.size());
        for (auto it = newTiles.cbegin(); it != newTiles.cend(); ++it) {
            const QUuid& tileUuid = it.key();
            const SurfaceTile& tile = it.value();
            vectorTileTextureToAppend_.push_back(std::make_pair(tileUuid, tile.getImageDataCRef()));
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// MosaicViewRenderImplementation
MosaicView::MosaicViewRenderImplementation::MosaicViewRenderImplementation() :
    colorTableTextureId_(0),
    tileGridVisible_(false),
    measLineVisible_(false)
{
#if defined(Q_OS_ANDROID) || defined(LINUX_ES)
    colorTableTextureType_ = GL_TEXTURE_2D;
#else
    colorTableTextureType_ = GL_TEXTURE_1D;
#endif
}

void MosaicView::MosaicViewRenderImplementation::render(QOpenGLFunctions *ctx,
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
        if (!textureId || !colorTableTextureId_) {
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
        glBindTexture(colorTableTextureType_, colorTableTextureId_);
        shaderProgram->setUniformValue("colorTable", 1);

        ctx->glDrawElements(GL_TRIANGLES, itm.getHeightIndicesRef().size(), GL_UNSIGNED_INT, itm.getHeightIndicesRef().constData());

        shaderProgram->disableAttributeArray(texCoordLoc);
        shaderProgram->disableAttributeArray(positionLoc);

        shaderProgram->release();
    }
}

void MosaicView::MosaicViewRenderImplementation::updateBounds()
{
    if (measLinesVertices_.isEmpty()) {
        m_bounds = Cube();
        return;
    }

    float z_max{ !std::isfinite(measLinesVertices_.first().z()) ? 0.f : measLinesVertices_.first().z() };
    float z_min{ z_max };
    float x_max{ !std::isfinite(measLinesVertices_.first().x()) ? 0.f : measLinesVertices_.first().x() };
    float x_min{ x_max };
    float y_max{ !std::isfinite(measLinesVertices_.first().y()) ? 0.f : measLinesVertices_.first().y() };
    float y_min{ y_max };

    for (const auto& itm: qAsConst(measLinesVertices_)) {
        z_min = std::min(z_min, !std::isfinite(itm.z()) ? 0.f : itm.z());
        z_max = std::max(z_max, !std::isfinite(itm.z()) ? 0.f : itm.z());
        x_min = std::min(x_min, !std::isfinite(itm.x()) ? 0.f : itm.x());
        x_max = std::max(x_max, !std::isfinite(itm.x()) ? 0.f : itm.x());
        y_min = std::min(y_min, !std::isfinite(itm.y()) ? 0.f : itm.y());
        y_max = std::max(y_max, !std::isfinite(itm.y()) ? 0.f : itm.y());
    }

    m_bounds = Cube(x_min, x_max, y_min, y_max, z_min - 20.f, z_max);
}

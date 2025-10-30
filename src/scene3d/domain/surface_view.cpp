#include "surface_view.h"

#include "draw_utils.h"
#include "text_renderer.h"
#include <QFile>


static inline QVector2D projectToScreen(const QVector3D& p, const QMatrix4x4& mvp, const QRect& viewport)
{
    const QVector4D clip = mvp * QVector4D(p, 1.0f);
    const float w = clip.w();

    if (qFuzzyIsNull(w)) {
        return QVector2D(NAN, NAN);
    }

    const QVector3D ndc(clip.x()/w, clip.y()/w, clip.z()/w);
    const float x = viewport.x() + (ndc.x() * 0.5f + 0.5f) * viewport.width();
    const float y = viewport.y() + (ndc.y() * 0.5f + 0.5f) * viewport.height();

    return QVector2D(x, y);
}

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

void SurfaceView::setMosaicTextureIdByTileId(const TileKey& tileId, GLuint textureId)
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

GLuint SurfaceView::getMosaicTextureIdByTileId(const TileKey& tileId) const
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

QVector<std::pair<TileKey, std::vector<uint8_t> > > SurfaceView::takeMosaicTileTextureToAppend()
{
    QMutexLocker lock(&mosaicTexTasksMutex_);

    QVector<std::pair<TileKey, std::vector<uint8_t>>> out;
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

std::vector<uint8_t> SurfaceView::takeSurfaceColorTableToAppend()
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

void SurfaceView::setLlaRef(LLARef llaRef)
{
    llaRef_ = llaRef;
}

void SurfaceView::saveVerticesToFile(const QString &path)
{
    auto* r = RENDER_IMPL(SurfaceView);
    if (!r) {
        qWarning() << "SurfaceView::saveVerticesToFile: no render impl";
        return;
    }

#ifdef Q_OS_ANDROID
    const QString filePath = path;
#else
    const QString filePath = QUrl(path).toLocalFile();
#endif

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return;
    }

    QTextStream out(&file);
    out.setLocale(QLocale::c());
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(8);
    out << "lat,lon,alt,x,y,z\n";

    for (auto it = r->tiles_.cbegin(); it != r->tiles_.cend(); ++it) {
        const SurfaceTile& tile = it.value();
        if (!tile.getIsInited())
            continue;

        const QVector<QVector3D>& verts = tile.getHeightVerticesCRef();
        const QVector<HeightType>& marks = tile.heightMarkVertices_;

        if (verts.isEmpty() || marks.isEmpty())
            continue;


        const QVector3D& v = verts[0]; // левая верхняя вершина (0,0) => idx = 0
        if (marks[0] == HeightType::kUndefined) {
            continue;
        }

        if (!std::isfinite(v.x()) || !std::isfinite(v.y()) || !std::isfinite(v.z())) {
            continue;
        }

        double lat = std::numeric_limits<double>::quiet_NaN();
        double lon = std::numeric_limits<double>::quiet_NaN();

        if (llaRef_.isInit) {
            NED ned(v.x(), v.y(), v.z());
            LLA lla(&ned, &llaRef_);
            if (std::isfinite(lla.latitude) && std::isfinite(lla.longitude)) {
                lat = lla.latitude;
                lon = lla.longitude;
            }
        }

        out << lat << "," << lon << "," << v.z() << ","
            << v.x() << "," << v.y() << "," << v.z() << "\n";
    }

    file.close();
}

bool SurfaceView::trySetMosaicTextureId(const TileKey &key, GLuint texId)
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        if (auto it = r->tiles_.find(key); it != r->tiles_.end()) {
            it.value().setMosaicTextureId(texId);
            Q_EMIT changed();
            return true;
        }
    }
    return false;
}

bool SurfaceView::hasTile(const TileKey &key) const
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        return r->tiles_.contains(key);
    }
    return false;
}

void SurfaceView::setTraceLines(const QVector3D &leftBeg, const QVector3D &leftEnd, const QVector3D &rightBeg, const QVector3D &rightEnd)
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        r->lastLeftLine_.resize(2);
        r->lastRightLine_.resize(2);
        r->lastLeftLine_[0]  = leftBeg;
        r->lastLeftLine_[1]  = leftEnd;
        r->lastRightLine_[0] = rightBeg;
        r->lastRightLine_[1] = rightEnd;
        Q_EMIT changed();
    }
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

    r->lastLeftLine_.clear();
    r->lastRightLine_.clear();

    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void SurfaceView::setTiles(const QHash<TileKey, SurfaceTile> &tiles, bool useTextures)
{
    //qDebug() << "SurfaceView::setTiles" << tiles.size();

    clear();

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

        r->updateBounds();

        Q_EMIT changed();
        Q_EMIT boundsChanged();
    }
}

void SurfaceView::setTilesIncremental(const QHash<TileKey, SurfaceTile> &tiles, const QSet<TileKey> &fullVisibleNow)
{
    auto* r = RENDER_IMPL(SurfaceView);
    if (!r) {
        return;
    }

    auto& cur = r->tiles_;

    // to del
    QSet<TileKey> toRemove;
    toRemove.reserve(cur.size());
    for (auto it = cur.cbegin(); it != cur.cend(); ++it) {
        if (!fullVisibleNow.contains(it.key())) {
            toRemove.insert(it.key());
        }
    }

    if (!toRemove.isEmpty()) {
        QMutexLocker lk(&mosaicTexTasksMutex_);
        for (const auto& k : toRemove) {
            if (auto it = cur.find(k); it != cur.end()) {
                if (GLuint oldId = it.value().getMosaicTextureId(); oldId) {
                    mosaicTileTextureToDelete_.append(oldId);
                }
                cur.erase(it);
            }

            mosaicTileTextureToAppend_.remove(k); // удалить все неактуальные задания на этот ключ
        }
    }

    // add/upd
    {
        QMutexLocker lk(&mosaicTexTasksMutex_);
        for (auto it = tiles.cbegin(); it != tiles.cend(); ++it) {
            const TileKey& key = it.key();
            const auto& inTile = it.value();

            auto curIt = cur.find(key);
            if (curIt == cur.end()) {
                cur.insert(key, inTile);
                if (inTile.updateHint() == UpdateHint::kAddOrUpdateTexture ||
                    inTile.updateHint() == UpdateHint::kUpdateTexture ||
                    !inTile.getMosaicImageDataCRef().empty()) {
                    mosaicTileTextureToAppend_.insert(key, inTile.getMosaicImageDataCRef());
                }
            }
            else {
                GLuint texId = curIt.value().getMosaicTextureId();
                curIt.value() = inTile;
                curIt.value().setMosaicTextureId(texId);

                if (inTile.updateHint() == UpdateHint::kUpdateTexture ||
                    inTile.updateHint() == UpdateHint::kAddOrUpdateTexture)
                {
                    mosaicTileTextureToAppend_.insert(key, curIt.value().getMosaicImageDataCRef());
                }
            }
        }
    }

    r->updateBounds();
    Q_EMIT changed();
    Q_EMIT boundsChanged();
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

void SurfaceView::setTextureTask(const std::vector<uint8_t> &textureTask)
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

void SurfaceView::removeTiles(const QSet<TileKey>& ids)
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

void SurfaceView::updateMosaicTileTextureTask(const QHash<TileKey, SurfaceTile>& newTiles)
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
    mVis_(false),
    traceWidth_(2.0f),
    traceVisible_(true)
{}

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
            glBindTexture(GL_TEXTURE_2D, mosaicColorTableTextureId_);
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

    auto lineProg = shaderProgramMap.value("static", nullptr);
    if (!lineProg) {
        qWarning() << "Shader program 'static' not found! Tile frames disabled.";
        return;
    }

    { // лучи
        if (traceVisible_ && lastLeftLine_.size() == 2 && lastRightLine_.size() == 2) {
            lineProg->bind();

            const int posLoc   = lineProg->attributeLocation("position");
            const int matLoc   = lineProg->uniformLocation("matrix");
            const int colorLoc = lineProg->uniformLocation("color");
            const int widthLoc = lineProg->uniformLocation("width");
            const int triLoc   = lineProg->uniformLocation("isTriangle");

            lineProg->setUniformValue(matLoc, mvp);
            lineProg->setUniformValue(triLoc, false);
            lineProg->setUniformValue(widthLoc, traceWidth_);

            lineProg->enableAttributeArray(posLoc);

            ctx->glLineWidth(traceWidth_);
            lineProg->setUniformValue(colorLoc, QVector4D(0.0f, 0.8f, 1.0f, 0.0f));
            lineProg->setAttributeArray(posLoc, lastLeftLine_.constData());
            ctx->glDrawArrays(GL_LINES, 0, 2);
            lineProg->setUniformValue(colorLoc, QVector4D(0.0f, 0.8f, 1.0f, 0.0f));
            lineProg->setAttributeArray(posLoc, lastRightLine_.constData());
            ctx->glDrawArrays(GL_LINES, 0, 2);
            ctx->glLineWidth(1.0);

            lineProg->disableAttributeArray(posLoc);
            lineProg->release();
        }
    }

    //return;
    // debug info
    // tile bounds
    {
        lineProg->bind();
        const int posLoc   = lineProg->attributeLocation("position");
        const int matLoc   = lineProg->uniformLocation("matrix");
        const int colorLoc = lineProg->uniformLocation("color");
        const int widthLoc = lineProg->uniformLocation("width");
        const int triLoc   = lineProg->uniformLocation("isTriangle");

        lineProg->setUniformValue(matLoc, mvp);
        lineProg->setUniformValue(colorLoc, QVector4D(1.0f,1.0f,1.0f,1.0f));
        lineProg->setUniformValue(widthLoc, 1.0f);
        lineProg->setUniformValue(triLoc, false);

        lineProg->enableAttributeArray(posLoc);

        ctx->glDisable(GL_DEPTH_TEST);

        QVector<QVector3D> rect(4);
        for (auto it = tiles_.cbegin(); it != tiles_.cend(); ++it) {
            const SurfaceTile& t = it.value();
            //if (!t.getIsInited()) {
            //    continue;
            //}

            float x0, x1, y0, y1;
            t.footprint(x0, x1, y0, y1);

            rect[0] = QVector3D(x0, y0, 0.0f);
            rect[1] = QVector3D(x1, y0, 0.0f);
            rect[2] = QVector3D(x1, y1, 0.0f);
            rect[3] = QVector3D(x0, y1, 0.0f);

            lineProg->setAttributeArray(posLoc, rect.constData());
            ctx->glDrawArrays(GL_LINE_LOOP, 0, 4);
        }

        ctx->glEnable(GL_DEPTH_TEST);

        lineProg->disableAttributeArray(posLoc);
        lineProg->setUniformValue(triLoc, false);
        lineProg->release();
    }

    {
        QRectF vport = DrawUtils::viewportRect(ctx);
        QMatrix4x4 textProjection;
        textProjection.ortho(vport.toRect());

        const float padX     = 4.0f;
        const float padY     = 4.0f;
        const float keyDY    = 14.0f;
        const float scaleStr = 0.4f;

        for (auto it = tiles_.cbegin(); it != tiles_.cend(); ++it) {
            const TileKey& key   = it.key();
            const SurfaceTile& t = it.value();

            //if (!t.getIsInited()) {
            //    continue;
            //}

            float x0, x1, y0, y1;
            t.footprint(x0, x1, y0, y1);

            const QVector3D originWorld(x0, y0, 0.0f);
            QVector2D scr = projectToScreen(originWorld, mvp, vport.toRect());
            if (!std::isfinite(scr.x()) || !std::isfinite(scr.y())) {
                continue;
            }

            { // origin
                QVector2D p = scr;
                p.setY(vport.height() - p.y() - padY);
                p.setX(p.x() + padX);
                const QString label = QStringLiteral("%1 %2") .arg(x0, 0, 'f', 1) .arg(y0, 0, 'f', 1);
                TextRenderer::instance().render(label, scaleStr, p, false, ctx, textProjection, shaderProgramMap);
            }

            { // key
                QString keyStr = QString("%1 %2 %3").arg(key.x).arg(key.y).arg(key.zoom);
                QVector2D p = scr;
                p.setY(p.y() + keyDY);
                p.setY(vport.height() - p.y() - padY);
                p.setX(p.x() + padX);
                TextRenderer::instance().render(keyStr, scaleStr, p, false, ctx, textProjection, shaderProgramMap);
            }
        }
    }
}

void SurfaceView::SurfaceViewRenderImplementation::updateBounds()
{
    if (tiles_.isEmpty()) {
        m_bounds = Cube();
        return;
    }

    bool anyTile = false;
    float xMin = 0.f, xMax = 0.f;
    float yMin = 0.f, yMax = 0.f;

    for (auto it = tiles_.cbegin(); it != tiles_.cend(); ++it) {
        const SurfaceTile& t = it.value();

        if (!t.getIsInited()) {
            continue;
        }

        float x0, x1, y0, y1;
        t.footprint(x0, x1, y0, y1);

        if (!anyTile) {
            xMin = x0; xMax = x1;
            yMin = y0; yMax = y1;
            anyTile = true;
        }
        else {
            if (x0 < xMin) xMin = x0;
            if (x1 > xMax) xMax = x1;
            if (y0 < yMin) yMin = y0;
            if (y1 > yMax) yMax = y1;
        }
    }

    if (!anyTile) {
        m_bounds = Cube();
        return;
    }

    m_bounds = Cube(xMin, xMax, yMin, yMax, 0, 0);
}

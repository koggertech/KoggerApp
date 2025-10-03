#include "mosaic_db.h"
#include <QFileInfo>
#include <QDir>
#include <QSqlError>
#include <QtEndian>
#include <QDebug>


static bool runWalCheckpoint(QSqlDatabase& db, const char* mode, int& busy, int& log, int& ckpt)
{
    QSqlQuery q(db);
    if (!q.exec(QString("PRAGMA wal_checkpoint(%1);").arg(mode))) {
        qWarning() << "wal_checkpoint" << mode << "failed:" << q.lastError();
        return false;
    }
    if (!q.next()) { // busy, log, checkpointed
        return false;
    }

    busy = q.value(0).toInt();
    log  = q.value(1).toInt();
    ckpt = q.value(2).toInt();

    q.finish();

    return true;
}

static QString makeXYOrClause(int pairCount) {
    QStringList parts;
    parts.reserve(pairCount);

    for (int i = 0; i < pairCount; ++i) {
        parts << "(x=? AND y=?)";
    }

    return parts.join(" OR ");
}

static QString dbPathForKlf(const QString& klfPath)
{
    QFileInfo fi(klfPath);
    const QString base = fi.completeBaseName();
    const QString dir  = fi.dir().absolutePath();
    return dir + "/" + base + ".mosaic.db";
}

MosaicDB::MosaicDB(const QString& klfPath, DbRole role, QObject* parent)
    : QObject(parent),
    klfPath_(klfPath),
    dbPath_(dbPathForKlf(klfPath)),
    role_(role)
{
    qRegisterMetaType<QList<DbTile>>("QList<DbTile>");
    qRegisterMetaType<QHash<TileKey,SurfaceTile>>("QHash<TileKey,SurfaceTile>");
    qRegisterMetaType<QVector<TileKey>>("QVector<TileKey>");

    const char* suffix = (role_ == DbRole::Reader ? "reader" : "writer");
    connName_ = QStringLiteral("mosaicdb-%1-%2").arg(reinterpret_cast<quintptr>(this)).arg(suffix);
}

MosaicDB::~MosaicDB()
{
    if (db_.isOpen()) db_.close();
}

bool MosaicDB::open()
{
    db_ = QSqlDatabase::addDatabase("QSQLITE", connName_);
    db_.setDatabaseName(dbPath_);

    // conn options
    QString opts = QString("QSQLITE_BUSY_TIMEOUT=%1;").arg(busyTimeoutMs_);
    if (role_ == DbRole::Reader) {
        opts += "QSQLITE_OPEN_READONLY=1;";
    }
    db_.setConnectOptions(opts);

    if (!db_.open()) {
        qWarning() << "MosaicDB open failed:" << db_.lastError();
        return false;
    }

    QSqlQuery q(db_);

    if (role_ == DbRole::Writer) {
        if (q.exec("PRAGMA journal_mode;") && q.next()) {
            const auto cur = q.value(0).toString();
            q.finish();
            if (cur.compare("wal", Qt::CaseInsensitive)) {
                QSqlQuery q2(db_);
                if (q2.exec("PRAGMA journal_mode=WAL;") && q2.next() && !q2.value(0).toString().compare("wal", Qt::CaseInsensitive)) {
                    //qDebug() << "db starts in wal mode";
                }
            }
        }

        q.exec("PRAGMA synchronous=NORMAL;");
        q.exec(QString("PRAGMA busy_timeout=%1;").arg(busyTimeoutMs_));
        q.exec("PRAGMA temp_store=MEMORY;");
        q.exec("PRAGMA wal_autocheckpoint=1000;");
        q.exec("PRAGMA journal_size_limit=134217728;");

        ensureSchema();

        emit schemaReady(); //
    }
    else {
        q.exec(QString("PRAGMA busy_timeout=%1;").arg(busyTimeoutMs_));
    }

    return true;
}

void MosaicDB::close()
{
    if (db_.isOpen()) {
        db_.close();
    }

    const QString name = db_.connectionName();
    db_ = QSqlDatabase();
    QSqlDatabase::removeDatabase(name);
}

void MosaicDB::finalizeAndClose()
{
    if (!db_.isOpen()) {
        return;
    }

    if (role_ == DbRole::Writer) { // writer — аккуратно удалить WAL
        int busy = 0;
        int log = 0;
        int ckpt = 0;
        runWalCheckpoint(db_, "RESTART", busy, log, ckpt);
        if (busy == 0 && log == ckpt) {
            runWalCheckpoint(db_, "TRUNCATE", busy, log, ckpt);
        }
    }

    db_.close();
    const QString name = db_.connectionName();
    db_ = QSqlDatabase(); // разлинковать объект от подключения
    QSqlDatabase::removeDatabase(name); // убрать из пула
}

void MosaicDB::checkAnyTileForZoom(int zoom)
{
    bool exists = false;

    if (db_.isOpen()) {
        QSqlQuery q(db_);

        if (q.prepare("SELECT 1 FROM tiles WHERE zoom=? LIMIT 1")) {
            q.addBindValue(zoom);
            if (!q.exec()) {
                qWarning() << "checkAnyTileForZoom exec:" << q.lastError();
            }
            else {
                exists = q.next();
            }
        }
        else {
            qWarning() << "checkAnyTileForZoom prepare:" << q.lastError();
        }

        q.finish();
    }

    emit anyTileForZoom(zoom, exists);
}

bool MosaicDB::ensureSchema()
{
    QSqlQuery q(db_);

    if (!q.exec(
            "CREATE TABLE IF NOT EXISTS tiles ("
            " zoom       INTEGER NOT NULL,"
            " x          INTEGER NOT NULL,"
            " y          INTEGER NOT NULL,"
            " tile_px    INTEGER NOT NULL,"
            " hm_ratio   INTEGER NOT NULL,"
            " origin_x   REAL    NOT NULL,"   /* НОВОЕ */
            " origin_y   REAL    NOT NULL,"   /* НОВОЕ */
            " has_mosaic INTEGER NOT NULL,"
            " mosaic_blob BLOB,"
            " has_height INTEGER NOT NULL,"
            " height_fmt INTEGER NOT NULL,"
            " height_blob BLOB,"
            " has_marks  INTEGER NOT NULL,"
            " marks_fmt  INTEGER NOT NULL,"
            " marks_blob BLOB,"
            " updated_at INTEGER NOT NULL,"
            " engine_ver INTEGER NOT NULL,"
            " PRIMARY KEY(zoom,x,y))")) {
        qWarning() << "tiles schema:" << q.lastError();
        return false;
    }
    return true;
}

// serialize
QByteArray MosaicDB::packRaw8(const std::vector<uint8_t>& v)
{
    QByteArray raw(reinterpret_cast<const char*>(v.data()), int(v.size()));
    return qCompress(raw, 6);
}

std::vector<uint8_t> MosaicDB::unpackRaw8(const QByteArray& blob)
{
    QByteArray raw = qUncompress(blob);
    std::vector<uint8_t> out(size_t(raw.size()));
    memcpy(out.data(), raw.constData(), out.size());
    return out;
}

QByteArray MosaicDB::packFloat32(const QVector<QVector3D>& verts, int hmRatio)
{
    const int side = hmRatio + 1;
    const int count = side * side;

    QByteArray raw; raw.resize(count * int(sizeof(float)));
    char* p = raw.data();

    for (int i = 0; i < count; ++i) {
        float z = verts[i].z();
        memcpy(p, &z, sizeof(float));
        p += sizeof(float);
    }

    return qCompress(raw, 6);
}

void MosaicDB::unpackFloat32(const QByteArray& blob, QVector<QVector3D>& verts, int hmRatio)
{
    QByteArray raw = qUncompress(blob);
    const int side = hmRatio + 1;
    const int count = side * side;

    verts.resize(count);
    const char* p = raw.constData();

    for (int i = 0; i < count; ++i) {
        float z;
        memcpy(&z, p, sizeof(float));
        p += sizeof(float);
        auto v = verts[i];
        v.setZ(z);
        verts[i] = v;
    }
}

QByteArray MosaicDB::packMarksU8(const QVector<HeightType>& marks)
{
    QByteArray raw; raw.resize(marks.size());
    char* p = raw.data();

    for (int i = 0; i < marks.size(); ++i) {
        uint8_t v = static_cast<uint8_t>(marks[i]);
        memcpy(p++, &v, 1);
    }

    return qCompress(raw, 6);
}

void MosaicDB::unpackMarksU8(const QByteArray& blob, QVector<HeightType>& marks)
{
    QByteArray raw = qUncompress(blob);
    marks.resize(raw.size());
    const char* p = raw.constData();

    for (int i = 0; i < raw.size(); ++i) {
        uint8_t v;
        memcpy(&v, p++, 1);
        marks[i] = static_cast<HeightType>(v);
    }
}

void MosaicDB::saveTiles(int engineVer, const QHash<TileKey, SurfaceTile>& tiles, bool useTextures, int tilePx, int hmRatio)
{
    QSqlQuery q(db_);
    db_.transaction();
    q.prepare(
        "INSERT INTO tiles("
        "zoom,x,y,tile_px,hm_ratio,origin_x,origin_y,"
        "has_mosaic,mosaic_blob,"
        "has_height,height_fmt,height_blob,"
        "has_marks,marks_fmt,marks_blob,"
        "updated_at,engine_ver)"
        "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        "ON CONFLICT(zoom,x,y) DO UPDATE SET "
        " tile_px=excluded.tile_px, hm_ratio=excluded.hm_ratio,"
        " origin_x=excluded.origin_x, origin_y=excluded.origin_y,"
        " has_mosaic=excluded.has_mosaic, mosaic_blob=excluded.mosaic_blob,"
        " has_height=excluded.has_height, height_fmt=excluded.height_fmt, height_blob=excluded.height_blob,"
        " has_marks=excluded.has_marks, marks_fmt=excluded.marks_fmt, marks_blob=excluded.marks_blob,"
        " updated_at=excluded.updated_at, engine_ver=excluded.engine_ver"
        );

    const qint64 now = QDateTime::currentSecsSinceEpoch();
    QVector<TileKey> savedKeys;
    savedKeys.reserve(tiles.size());

    for (auto it = tiles.cbegin(); it != tiles.cend(); ++it) {
        const TileKey& k = it.key();
        const SurfaceTile& t = it.value();

        // мозайка
        QByteArray mosaicBlob;
        const bool hasMosaic = useTextures && !t.getMosaicImageDataCRef().empty();
        if (hasMosaic) {
            mosaicBlob = packRaw8(t.getMosaicImageDataCRef());
        }

        // высоты
        QByteArray hBlob;
        const bool hasHeight = t.getIsInited() && !t.getHeightVerticesCRef().isEmpty();
        if (hasHeight) {
            hBlob = packFloat32(t.getHeightVerticesCRef(), hmRatio);
        }

        // метки
        QByteArray mBlob;
        bool hasMarks = false;
        if (t.getIsInited()) {
            auto marks = const_cast<SurfaceTile&>(t).getHeightMarkVerticesRef();
            if (!marks.isEmpty()) {
                hasMarks = true;
                mBlob = packMarksU8(marks);
            }
        }

        const auto org = t.getOrigin();

        q.addBindValue(k.zoom);
        q.addBindValue(k.x);
        q.addBindValue(k.y);
        q.addBindValue(tilePx);
        q.addBindValue(hmRatio);
        q.addBindValue(double(org.x()));   // origin_x
        q.addBindValue(double(org.y()));   // origin_y
        q.addBindValue(int(hasMosaic));
        q.addBindValue(mosaicBlob);
        q.addBindValue(int(hasHeight));
        q.addBindValue(0);                 // height_fmt = float32
        q.addBindValue(hBlob);
        q.addBindValue(int(hasMarks));
        q.addBindValue(0);                 // marks_fmt = u8
        q.addBindValue(mBlob);
        q.addBindValue(now);
        q.addBindValue(engineVer);

        if (!q.exec()) {
            qWarning() << "saveTiles exec:" << q.lastError() << "key" << k;
        }
        else {
            savedKeys.push_back(k);
        }

        q.finish();
    }

    db_.commit();

    checkpointIfWalTooBig(128ll * 1024 * 1024); // порог 128

    emit sendSavedKeys(savedKeys);
}

void MosaicDB::loadTilesForKeys(const QSet<TileKey> &keys)
{
    if (!db_.isOpen() || keys.isEmpty()) {
        emit tilesLoadedForKeys({});
        return;
    }

    QHash<int, QVector<TileKey>> groupedByZoom;
    groupedByZoom.reserve(keys.size());
    for (const auto& k : keys) {
        groupedByZoom[k.zoom].push_back(k);
    }

    QList<DbTile> out;
    out.reserve(keys.size());

    //qDebug() << "MosaicDB::loadTilesForKeys" << keys.size();

    for (auto it = groupedByZoom.cbegin(); it != groupedByZoom.cend(); ++it) {
        const int z = it.key();
        const QVector<TileKey>& list = it.value();

        for (int start = 0; start < list.size(); start += kMaxPairsPerBatch_) { // step by batch
            const int count = std::min(kMaxPairsPerBatch_, list.size() - start);

            const QString sql =
                QStringLiteral(
                    "SELECT x,y,zoom,origin_x,origin_y,tile_px,hm_ratio,"
                    "       mosaic_blob, height_blob, marks_blob,"
                    "       has_mosaic, has_height, has_marks "
                    "FROM tiles "
                    "WHERE zoom=? AND ( %1 );")
                    .arg(makeXYOrClause(count));

            QSqlQuery q(db_);
            if (!q.prepare(sql)) {
                qWarning() << "prepare failed:" << q.lastError();
                continue;
            }

            q.addBindValue(z);
            for (int i = 0; i < count; ++i) {
                const TileKey& k = list[start + i];
                q.addBindValue(k.x);
                q.addBindValue(k.y);
            }

            if (!q.exec()) {
                qWarning() << "exec failed:" << q.lastError();
                continue;
            }

            while (q.next()) {
                DbTile t;
                t.key.x    = q.value(0).toInt();
                t.key.y    = q.value(1).toInt();
                t.key.zoom = q.value(2).toInt();

                t.originX  = q.value(3).toFloat();
                t.originY  = q.value(4).toFloat();
                t.tilePx   = q.value(5).toInt();
                t.hmRatio  = q.value(6).toInt();

                t.mosaicBlob = q.value(7).toByteArray();
                t.heightBlob = q.value(8).toByteArray();
                t.marksBlob  = q.value(9).toByteArray();

                t.hasMosaic  = q.value(10).toInt() != 0;
                t.hasHeight  = q.value(11).toInt() != 0;
                t.hasMarks   = q.value(12).toInt() != 0;

                out.push_back(std::move(t));
            }

            q.finish();
        }
    }

    //qDebug() << "MosaicDB::loadTilesForKeys OUT" << out.size();

    emit tilesLoadedForKeys(out);
}

void MosaicDB::walCheckpointPassive()
{
    if (role_ != DbRole::Writer) {
        return;
    }
    int busy = 0;
    int log = 0;
    int ckpt = 0;

    if (runWalCheckpoint(db_, "PASSIVE", busy, log, ckpt)) {
        qDebug() << "[WAL] PASSIVE busy=" << busy << "log=" << log << "ckpt=" << ckpt;
    }
}

void MosaicDB::walCheckpointRestart()
{
    if (role_ != DbRole::Writer) {
        return;
    }

    int busy = 0;
    int log = 0;
    int ckpt = 0;

    if (runWalCheckpoint(db_, "RESTART", busy, log, ckpt)) {
        qDebug() << "[WAL] RESTART busy=" << busy << "log=" << log << "ckpt=" << ckpt;
    }
}

void MosaicDB::walCheckpointTruncate()
{
    if (role_ != DbRole::Writer) {
        return;
    }

    int busy = 0;
    int log = 0;
    int ckpt = 0;

    if (runWalCheckpoint(db_, "TRUNCATE", busy, log, ckpt)) {
        qDebug() << "[WAL] TRUNCATE busy=" << busy << "log=" << log << "ckpt=" << ckpt;
    }
}

void MosaicDB::checkpointIfWalTooBig(uint64_t limitBytes)
{
    if (role_ != DbRole::Writer) {
        return;
    }

    QFileInfo fi(dbPath_ + "-wal");
    if (!fi.exists()) {
        return;
    }

    const uint64_t sz = fi.size();
    if (sz <= limitBytes) {
        return;
    }

    walCheckpointPassive();

    QFileInfo fi2(dbPath_ + "-wal"); // если wal всё ещё большой — ждём, пока текущие читатели отпустят снапшоты
    if (fi2.exists() && static_cast<uint64_t>(fi2.size()) > limitBytes) {
        walCheckpointRestart();
    }
}

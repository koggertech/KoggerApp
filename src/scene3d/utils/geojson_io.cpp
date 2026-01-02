#include "geojson_io.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QUuid>

#include "geojson_style.h"

static QString makeFeatureId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

static bool parseCoord(const QJsonValue& v, GeoJsonCoord* out, QString* err)
{
    if (!v.isArray()) {
        if (err) *err = QStringLiteral("coord must be array");
        return false;
    }

    const auto arr = v.toArray();
    if (arr.size() < 2 || arr.size() > 3) {
        if (err) *err = QStringLiteral("coord must be [lon, lat] or [lon, lat, z]");
        return false;
    }
    if (!arr.at(0).isDouble() || !arr.at(1).isDouble()) {
        if (err) *err = QStringLiteral("coord lon/lat must be numbers");
        return false;
    }

    out->lon = arr.at(0).toDouble();
    out->lat = arr.at(1).toDouble();
    out->z = 0.0;
    out->hasZ = false;
    if (arr.size() == 3) {
        if (!arr.at(2).isDouble()) {
            if (err) *err = QStringLiteral("coord z must be number");
            return false;
        }
        out->z = arr.at(2).toDouble();
        out->hasZ = true;
    }

    return true;
}

static bool parseCoords1(const QJsonValue& v, QVector<GeoJsonCoord>* out, QString* err, int minCount)
{
    if (!v.isArray()) {
        if (err) *err = QStringLiteral("coordinates must be array");
        return false;
    }

    const auto arr = v.toArray();
    if (arr.size() < minCount) {
        if (err) *err = QStringLiteral("not enough vertices");
        return false;
    }

    out->clear();
    out->reserve(arr.size());
    for (const auto& it : arr) {
        GeoJsonCoord c;
        if (!parseCoord(it, &c, err)) {
            return false;
        }
        out->push_back(c);
    }
    return true;
}

static bool parsePolygon(const QJsonValue& v, QVector<GeoJsonCoord>* out, QString* err)
{
    if (!v.isArray()) {
        if (err) *err = QStringLiteral("polygon coordinates must be array");
        return false;
    }

    const auto rings = v.toArray();
    if (rings.isEmpty()) {
        if (err) *err = QStringLiteral("polygon must have a ring");
        return false;
    }
    if (rings.size() > 1) {
        if (err) *err = QStringLiteral("polygon holes are not supported");
        return false;
    }

    if (!parseCoords1(rings.at(0), out, err, 4)) {
        return false;
    }

    if (out->size() < 4) {
        if (err) *err = QStringLiteral("polygon ring must have 4+ points");
        return false;
    }

    if (out->first() != out->last()) {
        if (err) *err = QStringLiteral("polygon ring must be closed");
        return false;
    }

    return true;
}

static QString parseFeatureId(const QJsonObject& fObj)
{
    const QJsonValue idVal = fObj.value(QStringLiteral("id"));
    if (idVal.isString()) {
        return idVal.toString();
    }
    if (idVal.isDouble()) {
        return QString::number(idVal.toDouble());
    }
    return makeFeatureId();
}

static QJsonArray writeCoord(const GeoJsonCoord& c)
{
    QJsonArray arr;
    arr.append(c.lon);
    arr.append(c.lat);
    if (c.hasZ) {
        arr.append(c.z);
    }
    return arr;
}

namespace GeoJsonIO
{
Result loadFromFile(const QString& path)
{
    Result r;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        r.error = QStringLiteral("load: cannot open file");
        return r;
    }

    const QByteArray bytes = f.readAll();
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &pe);
    if (doc.isNull() || pe.error != QJsonParseError::NoError) {
        r.error = QStringLiteral("load: invalid JSON: ") + pe.errorString();
        return r;
    }
    if (!doc.isObject()) {
        r.error = QStringLiteral("load: root must be object");
        return r;
    }

    QString err;
    if (!parseFeatureCollection(doc.object(), &r.doc, &err)) {
        r.error = err;
        return r;
    }

    r.ok = true;
    return r;
}

bool saveToFile(const QString& path, const GeoJsonDocument& doc, QString* outError)
{
    QJsonDocument out(writeFeatureCollection(doc));
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (outError) *outError = QStringLiteral("save: cannot open file for writing");
        return false;
    }

    const QByteArray bytes = out.toJson(QJsonDocument::Indented);
    if (f.write(bytes) != bytes.size()) {
        if (outError) *outError = QStringLiteral("save: short write");
        return false;
    }
    return true;
}

bool parseFeatureCollection(const QJsonObject& root, GeoJsonDocument* outDoc, QString* outError)
{
    if (!outDoc) {
        if (outError) *outError = QStringLiteral("parse: output doc is null");
        return false;
    }

    if (root.value(QStringLiteral("type")).toString() != QStringLiteral("FeatureCollection")) {
        if (outError) *outError = QStringLiteral("parse: type must be FeatureCollection");
        return false;
    }

    const QJsonValue featuresVal = root.value(QStringLiteral("features"));
    if (!featuresVal.isArray()) {
        if (outError) *outError = QStringLiteral("parse: features must be array");
        return false;
    }

    outDoc->features.clear();
    const auto featuresArr = featuresVal.toArray();
    outDoc->features.reserve(featuresArr.size());

    for (const auto& fv : featuresArr) {
        if (!fv.isObject()) {
            if (outError) *outError = QStringLiteral("parse: feature must be object");
            return false;
        }

        const QJsonObject fo = fv.toObject();
        if (fo.value(QStringLiteral("type")).toString() != QStringLiteral("Feature")) {
            if (outError) *outError = QStringLiteral("parse: feature type must be Feature");
            return false;
        }

        const QJsonValue geomVal = fo.value(QStringLiteral("geometry"));
        if (!geomVal.isObject()) {
            if (outError) *outError = QStringLiteral("parse: geometry must be object");
            return false;
        }

        const QJsonObject go = geomVal.toObject();
        const QString gtype = go.value(QStringLiteral("type")).toString();
        const QJsonValue coordsVal = go.value(QStringLiteral("coordinates"));

        GeoJsonFeature f;
        f.id = parseFeatureId(fo);

        if (gtype == QStringLiteral("Point")) {
            f.geomType = GeoJsonGeometryType::Point;
            GeoJsonCoord c;
            if (!parseCoord(coordsVal, &c, outError)) {
                return false;
            }
            f.coords = {c};
        } else if (gtype == QStringLiteral("LineString")) {
            f.geomType = GeoJsonGeometryType::LineString;
            if (!parseCoords1(coordsVal, &f.coords, outError, 2)) {
                return false;
            }
        } else if (gtype == QStringLiteral("Polygon")) {
            f.geomType = GeoJsonGeometryType::Polygon;
            if (!parsePolygon(coordsVal, &f.coords, outError)) {
                return false;
            }
        } else {
            if (outError) *outError = QStringLiteral("parse: unsupported geometry type");
            return false;
        }

        const QJsonValue propsVal = fo.value(QStringLiteral("properties"));
        if (propsVal.isObject()) {
            f.properties = propsVal.toObject();
        } else {
            f.properties = QJsonObject();
        }

        const QJsonValue nameVal = f.properties.value(QStringLiteral("name"));
        if (nameVal.isString()) {
            f.name = nameVal.toString();
        }

        f.style = GeoJsonCss::parseStyle(f.properties);
        outDoc->features.push_back(std::move(f));
    }

    return true;
}

QJsonObject writeFeatureCollection(const GeoJsonDocument& doc)
{
    QJsonObject root;
    root.insert(QStringLiteral("type"), QStringLiteral("FeatureCollection"));

    QJsonArray features;
    for (const auto& f : doc.features) {
        QJsonObject fo;
        fo.insert(QStringLiteral("type"), QStringLiteral("Feature"));
        if (!f.id.isEmpty()) {
            fo.insert(QStringLiteral("id"), f.id);
        }

        QJsonObject props = f.properties;
        if (!f.name.isEmpty()) {
            props.insert(QStringLiteral("name"), f.name);
        } else {
            props.remove(QStringLiteral("name"));
        }
        GeoJsonCss::writeStyle(props, f.style);
        fo.insert(QStringLiteral("properties"), props);

        QJsonObject geom;
        switch (f.geomType) {
        case GeoJsonGeometryType::Point:
            geom.insert(QStringLiteral("type"), QStringLiteral("Point"));
            if (!f.coords.isEmpty()) {
                geom.insert(QStringLiteral("coordinates"), writeCoord(f.coords.first()));
            } else {
                geom.insert(QStringLiteral("coordinates"), QJsonArray());
            }
            break;
        case GeoJsonGeometryType::LineString: {
            geom.insert(QStringLiteral("type"), QStringLiteral("LineString"));
            QJsonArray arr;
            for (const auto& c : f.coords) {
                arr.append(writeCoord(c));
            }
            geom.insert(QStringLiteral("coordinates"), arr);
            break;
        }
        case GeoJsonGeometryType::Polygon: {
            geom.insert(QStringLiteral("type"), QStringLiteral("Polygon"));
            QJsonArray ring;
            QVector<GeoJsonCoord> coords = f.coords;
            if (coords.size() >= 3 && coords.first() != coords.last()) {
                coords.push_back(coords.first());
            }
            for (const auto& c : coords) {
                ring.append(writeCoord(c));
            }
            QJsonArray rings;
            rings.append(ring);
            geom.insert(QStringLiteral("coordinates"), rings);
            break;
        }
        }

        fo.insert(QStringLiteral("geometry"), geom);
        features.append(fo);
    }

    root.insert(QStringLiteral("features"), features);
    return root;
}
} // namespace GeoJsonIO

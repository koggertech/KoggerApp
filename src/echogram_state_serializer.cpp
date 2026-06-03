#include "echogram_state_serializer.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include "scene2d/qPlot2D.h"

EchogramStateSerializer::EchogramStateSerializer(QObject* parent)
    : QObject(parent)
{
}

QString EchogramStateSerializer::lastError() const
{
    return lastError_;
}

void EchogramStateSerializer::setLastError(const QString& text) const
{
    if (lastError_ == text) {
        return;
    }
    lastError_ = text;
    emit const_cast<EchogramStateSerializer*>(this)->lastErrorChanged();
}

QString EchogramStateSerializer::serialize(QObject* echogram) const
{
    auto* p = qobject_cast<qPlot2D*>(echogram);
    if (!p) {
        setLastError(QStringLiteral("serialize: object is not a qPlot2D"));
        return QString();
    }

    QJsonObject o;
    o[QStringLiteral("_v")] = kVersion;

    o[QStringLiteral("ch1")]      = p->plotDatasetChannelName();
    o[QStringLiteral("ch2")]      = p->plotDatasetChannel2Name();

    o[QStringLiteral("egVis")]    = p->getEchogramVisible();
    o[QStringLiteral("egTheme")]  = p->getThemeId();
    o[QStringLiteral("egComp")]   = p->getEchogramCompensation();
    o[QStringLiteral("egLow")]    = static_cast<double>(p->getEchogramLowLevel());
    o[QStringLiteral("egHigh")]   = static_cast<double>(p->getEchogramHighLevel());

    o[QStringLiteral("btVis")]    = p->getBottomTrackVisible();
    o[QStringLiteral("btText")]   = p->getBottomTrackDepthTextVisible();
    o[QStringLiteral("btTheme")]  = p->getBottomTrackTheme();

    o[QStringLiteral("rfVis")]    = p->getRangefinderVisible();
    o[QStringLiteral("rfText")]   = p->getRangefinderDepthTextVisible();
    o[QStringLiteral("rfTheme")]  = p->getRangefinderTheme();

    o[QStringLiteral("attVis")]   = p->getAttitudeVisible();
    o[QStringLiteral("tempVis")]  = p->getTemperatureVisible();

    o[QStringLiteral("dbVis")]    = p->getDopplerBeamVisible();
    o[QStringLiteral("dbFilter")] = p->getDopplerBeamFilter();
    o[QStringLiteral("diVis")]    = p->getDopplerInstrumentVisible();
    o[QStringLiteral("diFilter")] = p->getDopplerInstrumentFilter();

    o[QStringLiteral("dvlVis")]   = p->getDVLLegendVisible();
    o[QStringLiteral("dvlPos")]   = p->getDVLLegendPosition();

    o[QStringLiteral("aaVis")]    = p->getAcousticAngleVisible();
    o[QStringLiteral("gnssVis")]  = p->getGNSSVisible();

    o[QStringLiteral("gridNum")]  = p->getGridVerticalNumber();
    o[QStringLiteral("gridFill")] = p->getGridFillWidth();
    o[QStringLiteral("gridInv")]  = p->getGridInvert();

    o[QStringLiteral("angVis")]   = p->getAngleVisibility();
    o[QStringLiteral("angRange")] = p->getAngleRange();
    o[QStringLiteral("velVis")]   = p->getVelocityVisible();
    o[QStringLiteral("velRange")] = static_cast<double>(p->getVelocityRange());

    o[QStringLiteral("distMode")] = p->getDistanceAutoRange();
    o[QStringLiteral("horiz")]    = p->isHorizontal();

    o[QStringLiteral("loupeVis")]  = p->getLoupeVisible();
    o[QStringLiteral("loupeSize")] = p->getLoupeSize();
    o[QStringLiteral("loupeZoom")] = p->getLoupeZoom();

    setLastError(QString());

    const QByteArray json = QJsonDocument(o).toJson(QJsonDocument::Compact);
    return QString::fromLatin1(json.toBase64());
}

bool EchogramStateSerializer::deserialize(QObject* echogram, const QString& state)
{
    auto* p = qobject_cast<qPlot2D*>(echogram);
    if (!p) {
        setLastError(QStringLiteral("deserialize: object is not a qPlot2D"));
        return false;
    }

    const QByteArray raw = QByteArray::fromBase64(state.toLatin1());
    QJsonParseError perr{};
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &perr);
    if (perr.error != QJsonParseError::NoError) {
        setLastError(QStringLiteral("deserialize: invalid JSON: %1").arg(perr.errorString()));
        return false;
    }
    if (!doc.isObject()) {
        setLastError(QStringLiteral("deserialize: root is not a JSON object"));
        return false;
    }

    const QJsonObject o = doc.object();
    const int version = o.value(QStringLiteral("_v")).toInt(-1);
    if (version != kVersion) {
        setLastError(QStringLiteral("deserialize: unsupported version %1").arg(version));
        return false;
    }

    auto getB = [&](const char* k, bool def) {
        return o.contains(QLatin1String(k)) ? o.value(QLatin1String(k)).toBool(def) : def;
    };
    auto getI = [&](const char* k, int def) {
        return o.contains(QLatin1String(k)) ? o.value(QLatin1String(k)).toInt(def) : def;
    };
    auto getD = [&](const char* k, double def) {
        return o.contains(QLatin1String(k)) ? o.value(QLatin1String(k)).toDouble(def) : def;
    };
    auto getS = [&](const char* k, const QString& def) {
        return o.contains(QLatin1String(k)) ? o.value(QLatin1String(k)).toString(def) : def;
    };

    p->plotDatasetChannelFromStrings(getS("ch1", p->plotDatasetChannelName()),
                                     getS("ch2", p->plotDatasetChannel2Name()));

    p->plotEchogramVisible(getB("egVis", p->getEchogramVisible()));
    p->plotEchogramTheme(getI("egTheme", p->getThemeId()));
    p->plotEchogramCompensation(getI("egComp", p->getEchogramCompensation()));
    p->plotEchogramSetLevels(static_cast<float>(getD("egLow", p->getEchogramLowLevel())),
                             static_cast<float>(getD("egHigh", p->getEchogramHighLevel())));

    p->plotBottomTrackVisible(getB("btVis", p->getBottomTrackVisible()));
    p->plotBottomTrackDepthTextVisible(getB("btText", p->getBottomTrackDepthTextVisible()));
    p->plotBottomTrackTheme(getI("btTheme", p->getBottomTrackTheme()));

    p->plotRangefinderVisible(getB("rfVis", p->getRangefinderVisible()));
    p->plotRangefinderDepthTextVisible(getB("rfText", p->getRangefinderDepthTextVisible()));
    p->plotRangefinderTheme(getI("rfTheme", p->getRangefinderTheme()));

    p->plotAttitudeVisible(getB("attVis", p->getAttitudeVisible()));
    p->plotTemperatureVisible(getB("tempVis", p->getTemperatureVisible()));

    p->plotDopplerBeamVisible(getB("dbVis", p->getDopplerBeamVisible()),
                              getI("dbFilter", p->getDopplerBeamFilter()));
    p->plotDopplerInstrumentVisible(getB("diVis", p->getDopplerInstrumentVisible()),
                                    getI("diFilter", p->getDopplerInstrumentFilter()));

    p->plotDVLLegendVisible(getB("dvlVis", p->getDVLLegendVisible()));
    p->plotDVLLegendPosition(getI("dvlPos", p->getDVLLegendPosition()));

    p->plotAcousticAngleVisible(getB("aaVis", p->getAcousticAngleVisible()));
    p->plotGNSSVisible(getB("gnssVis", p->getGNSSVisible()), 1);

    p->plotGridVerticalNumber(getI("gridNum", p->getGridVerticalNumber()));
    p->plotGridFillWidth(getB("gridFill", p->getGridFillWidth()));
    p->plotGridInvert(getB("gridInv", p->getGridInvert()));

    p->plotAngleVisibility(getB("angVis", p->getAngleVisibility()));
    p->plotAngleRange(getI("angRange", p->getAngleRange()));
    p->plotVelocityVisible(getB("velVis", p->getVelocityVisible()));
    p->plotVelocityRange(static_cast<float>(getD("velRange", p->getVelocityRange())));

    p->plotDistanceAutoRange(getI("distMode", p->getDistanceAutoRange()));
    p->setHorizontal(getB("horiz", p->isHorizontal()));

    p->plotLoupeVisible(getB("loupeVis", p->getLoupeVisible()));
    p->plotLoupeSize(getI("loupeSize", p->getLoupeSize()));
    p->plotLoupeZoom(getI("loupeZoom", p->getLoupeZoom()));

    p->plotUpdate();

    setLastError(QString());
    return true;
}

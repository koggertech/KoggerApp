#include "hotkeys_manager.h"

#include <QFile>
#include <QDir>
#include <QDebug>
#include <QStandardPaths>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>


// default hotkeys template
#if defined(Q_OS_LINUX)
const char* HotkeysManager::s_defaultHotkeysXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<Hotkeys>
    <Hotkey functionName="closeSettings"     scanCode="9"                  description="close connection/settings menu"/>
    <Hotkey functionName="horScrollRight"    scanCode="38" parameter="100" description="horizontal scroll of echogram to the right"/>
    <Hotkey functionName="horScrollLeft"     scanCode="24" parameter="100" description="horizontal scroll of echogram to the left"/>
    <Hotkey functionName="verScrollUp"       scanCode="25" parameter="100" description="vertical scroll of echogram up"/>
    <Hotkey functionName="verScrollDown"     scanCode="39" parameter="100" description="vertical scroll of echogram down"/>
    <Hotkey functionName="verZoomIn"         scanCode="40" parameter="100" description="echogram vertical zoom in"/>
    <Hotkey functionName="verZoomOut"        scanCode="26" parameter="100" description="echogram vertical zoom out"/>
    <Hotkey functionName="increaseLowLevel"  scanCode="27" parameter="1"   description="raise the lower slider of the echogram brightness"/>
    <Hotkey functionName="decreaseLowLevel"  scanCode="41" parameter="1"   description="lower the lower slider of the echogram brightness"/>
    <Hotkey functionName="increaseHighLevel" scanCode="28" parameter="1"   description="raise the upper slider of the echogram brightness"/>
    <Hotkey functionName="decreaseHighLevel" scanCode="42" parameter="1"   description="lower the upper slider of the echogram brightness"/>
    <Hotkey functionName="clickConnections"  scanCode="56"                 description="click connection button"/>
    <Hotkey functionName="clickSettings"     scanCode="57"                 description="click settings button"/>
    <Hotkey functionName="click3D"           scanCode="52"                 description="click 3D button"/>
    <Hotkey functionName="click2D"           scanCode="53"                 description="click 2D button"/>
    <Hotkey functionName="prevTheme"         scanCode="54"                 description="switch the echogram theme to the previous one"/>
    <Hotkey functionName="nextTheme"         scanCode="55"                 description="switch the echogram theme to the next one"/>
    <Hotkey functionName="toggleFullScreen"  scanCode="95"                 description="toggle fullscreen"/>
    <Hotkey functionName="openFile"          scanCode="76"                 description="open file"/>
    <Hotkey functionName="openFileDialog"    scanCode="96"                 description="open file dialog"/>
    <Hotkey functionName="closeFile"         scanCode="75"                 description="close file"/>
    <Hotkey functionName="updateBottomTrack" scanCode="72"                 description="update bottom track"/>
    <Hotkey functionName="updateMosaic"      scanCode="74"                 description="update mosaic"/>
    <Hotkey functionName="scene3dZoomIn"     scanCode="29"                 description="3D zoom in"/>
    <Hotkey functionName="scene3dZoomOut"    scanCode="43"                 description="3D zoom out"/>
    <Hotkey functionName="mosaicPrevTheme"   scanCode="30"                 description="mosaic previous theme"/>
    <Hotkey functionName="mosaicNextTheme"   scanCode="44"                 description="mosaic next theme"/>
    <Hotkey functionName="mosaicLowLevelUp"  scanCode="31" parameter="1"   description="mosaic lower level up"/>
    <Hotkey functionName="mosaicLowLevelDown" scanCode="45" parameter="1"  description="mosaic lower level down"/>
    <Hotkey functionName="mosaicHighLevelUp" scanCode="32" parameter="1"  description="mosaic upper level up"/>
    <Hotkey functionName="mosaicHighLevelDown" scanCode="46" parameter="1" description="mosaic upper level down"/>
    <Hotkey functionName="surfacePrevTheme"  scanCode="33"                 description="surface previous theme"/>
    <Hotkey functionName="surfaceNextTheme"  scanCode="47"                 description="surface next theme"/>
    <Hotkey functionName="surfaceStepDown"   scanCode="34" parameter="1"   description="surface isobaths step down"/>
    <Hotkey functionName="surfaceStepUp"     scanCode="48" parameter="1"   description="surface isobaths step up"/>
    <Hotkey functionName="toggleBottomTrack3D" scanCode="50"               description="toggle 3D bottom track"/>
    <Hotkey functionName="toggleIsobaths3D"  scanCode="59"                 description="toggle 3D isobaths"/>
    <Hotkey functionName="toggleMosaic3D"    scanCode="60"                 description="toggle 3D mosaic"/>
    <Hotkey functionName="cameraShiftXMinus3D" scanCode="10"               description="3D camera shift x minus"/>
    <Hotkey functionName="cameraShiftXPlus3D"  scanCode="11"               description="3D camera shift x plus"/>
    <Hotkey functionName="cameraShiftYMinus3D" scanCode="12"               description="3D camera shift y minus"/>
    <Hotkey functionName="cameraShiftYPlus3D"  scanCode="13"               description="3D camera shift y plus"/>
    <Hotkey functionName="resetCameraTop3D"    scanCode="14"               description="reset 3D camera to top view"/>
    <Hotkey functionName="cameraShiftZMinus3D" scanCode="15"               description="3D vertical scale down, Ctrl+wheel"/>
    <Hotkey functionName="cameraShiftZPlus3D"  scanCode="16"               description="3D vertical scale up, Ctrl+wheel"/>
    <Hotkey functionName="toggleEchogramType" scanCode="61"                 description="toggle echogram type raw/side-scan"/>
</Hotkeys>
)";
#else
const char* HotkeysManager::s_defaultHotkeysXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<Hotkeys>
    <Hotkey functionName="closeSettings"     scanCode="1"                  description="close connection/settings menu"/>
    <Hotkey functionName="horScrollRight"    scanCode="30" parameter="100" description="horizontal scroll of echogram to the right"/>
    <Hotkey functionName="horScrollLeft"     scanCode="16" parameter="100" description="horizontal scroll of echogram to the left"/>
    <Hotkey functionName="verScrollUp"       scanCode="17" parameter="100" description="vertical scroll of echogram up"/>
    <Hotkey functionName="verScrollDown"     scanCode="31" parameter="100" description="vertical scroll of echogram down"/>
    <Hotkey functionName="verZoomIn"         scanCode="32" parameter="100" description="echogram vertical zoom in"/>
    <Hotkey functionName="verZoomOut"        scanCode="18" parameter="100" description="echogram vertical zoom out"/>
    <Hotkey functionName="increaseLowLevel"  scanCode="19" parameter="1"   description="raise the lower slider of the echogram brightness"/>
    <Hotkey functionName="decreaseLowLevel"  scanCode="33" parameter="1"   description="lower the lower slider of the echogram brightness"/>
    <Hotkey functionName="increaseHighLevel" scanCode="20" parameter="1"   description="raise the upper slider of the echogram brightness"/>
    <Hotkey functionName="decreaseHighLevel" scanCode="34" parameter="1"   description="lower the upper slider of the echogram brightness"/>
    <Hotkey functionName="clickConnections"  scanCode="48"                 description="click connection button"/>
    <Hotkey functionName="clickSettings"     scanCode="49"                 description="click settings button"/>
    <Hotkey functionName="click3D"           scanCode="44"                 description="click 3D button"/>
    <Hotkey functionName="click2D"           scanCode="45"                 description="click 2D button"/>
    <Hotkey functionName="prevTheme"         scanCode="46"                 description="switch the echogram theme to the previous one"/>
    <Hotkey functionName="nextTheme"         scanCode="47"                 description="switch the echogram theme to the next one"/>
    <Hotkey functionName="toggleFullScreen"  scanCode="87"                 description="toggle fullscreen"/>
    <Hotkey functionName="openFile"          scanCode="68"                 description="open file"/>
    <Hotkey functionName="openFileDialog"    scanCode="88"                 description="open file dialog"/>
    <Hotkey functionName="closeFile"         scanCode="67"                 description="close file"/>
    <Hotkey functionName="updateBottomTrack" scanCode="64"                 description="update bottom track"/>
    <Hotkey functionName="updateMosaic"      scanCode="66"                 description="update mosaic"/>
    <Hotkey functionName="scene3dZoomIn"     scanCode="21"                 description="3D zoom in"/>
    <Hotkey functionName="scene3dZoomOut"    scanCode="35"                 description="3D zoom out"/>
    <Hotkey functionName="mosaicPrevTheme"   scanCode="22"                 description="mosaic previous theme"/>
    <Hotkey functionName="mosaicNextTheme"   scanCode="36"                 description="mosaic next theme"/>
    <Hotkey functionName="mosaicLowLevelUp"  scanCode="23" parameter="1"   description="mosaic lower level up"/>
    <Hotkey functionName="mosaicLowLevelDown" scanCode="37" parameter="1"  description="mosaic lower level down"/>
    <Hotkey functionName="mosaicHighLevelUp" scanCode="24" parameter="1"  description="mosaic upper level up"/>
    <Hotkey functionName="mosaicHighLevelDown" scanCode="38" parameter="1" description="mosaic upper level down"/>
    <Hotkey functionName="surfacePrevTheme"  scanCode="25"                 description="surface previous theme"/>
    <Hotkey functionName="surfaceNextTheme"  scanCode="39"                 description="surface next theme"/>
    <Hotkey functionName="surfaceStepDown"   scanCode="26" parameter="1"   description="surface isobaths step down"/>
    <Hotkey functionName="surfaceStepUp"     scanCode="40" parameter="1"   description="surface isobaths step up"/>
    <Hotkey functionName="toggleBottomTrack3D" scanCode="50"               description="toggle 3D bottom track"/>
    <Hotkey functionName="toggleIsobaths3D"  scanCode="51"                 description="toggle 3D isobaths"/>
    <Hotkey functionName="toggleMosaic3D"    scanCode="52"                 description="toggle 3D mosaic"/>
    <Hotkey functionName="cameraShiftXMinus3D" scanCode="2"                description="3D camera shift x minus"/>
    <Hotkey functionName="cameraShiftXPlus3D"  scanCode="3"                description="3D camera shift x plus"/>
    <Hotkey functionName="cameraShiftYMinus3D" scanCode="4"                description="3D camera shift y minus"/>
    <Hotkey functionName="cameraShiftYPlus3D"  scanCode="5"                description="3D camera shift y plus"/>
    <Hotkey functionName="resetCameraTop3D"    scanCode="6"                description="reset 3D camera to top view"/>
    <Hotkey functionName="cameraShiftZMinus3D" scanCode="7"                description="3D vertical scale down, Ctrl+wheel"/>
    <Hotkey functionName="cameraShiftZPlus3D"  scanCode="8"                description="3D vertical scale up, Ctrl+wheel"/>
    <Hotkey functionName="toggleEchogramType" scanCode="53"                 description="toggle echogram type raw/side-scan"/>
</Hotkeys>
)";
#endif


HotkeysManager::HotkeysManager()
{
    ensureDefaultHotkeysFile();
}

void HotkeysManager::ensureDefaultHotkeysFile() const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);

    QString filePath = configDir + "/hotkeys.xml";
    QFile file(filePath);

    if (!file.exists()) {
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(s_defaultHotkeysXml);
            file.close();
            qDebug() << "Created default hotkeys.xml in" << filePath;
        }
        else {
            qWarning() << "Cannot create hotkeys.xml in" << filePath;
        }
    }
    else {
        QList<HotkeyData> existingList = parseHotkeysFromFile(filePath);
        QList<HotkeyData> defaultList = parseHotkeysFromString(QString::fromUtf8(s_defaultHotkeysXml));

        QSet<QString> existingFunctions;
        for (auto it = existingList.begin(); it != existingList.end(); ++it) {
            existingFunctions.insert(it->functionName);
        }

        bool wasModified = false;
        for (auto it = defaultList.begin(); it != defaultList.end(); ++it) {
            if (!existingFunctions.contains(it->functionName)) {
                existingList.append(*it);
                existingFunctions.insert(it->functionName);
                wasModified = true;
            }
        }

        if (wasModified) {
            if (saveHotkeysToFile(existingList, filePath)) {
                qDebug() << "hotkeys.xml was updated with new default hotkeys in" << filePath;
            }
        }
    }
}

QMap<uint32_t, HotkeyData> HotkeysManager::loadHotkeysMapping() const
{
    QMap<uint32_t, HotkeyData> hotkeysMap;

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString filePath  = configDir + "/hotkeys.xml";

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open hotkeys.xml at" << filePath;
        return hotkeysMap;
    }

    QXmlStreamReader xml(&file);

    while (!xml.atEnd() && !xml.hasError()) {
        auto token = xml.readNext();
        if (token == QXmlStreamReader::StartElement && xml.name() == "Hotkey") {
            QXmlStreamAttributes attrs = xml.attributes();

            QString scancodeStr  = attrs.value("scanCode").toString();
            bool okSC = false;
            uint32_t scancode = scancodeStr.toUInt(&okSC);

            QString functionName = attrs.value("functionName").toString();

            QString paramStr = attrs.value("parameter").toString();
            bool okParam = false;
            int paramVal = paramStr.toInt(&okParam);

            if (okSC && scancode != 0 && !functionName.isEmpty()) {
                HotkeyData data;
                data.functionName = functionName;
                data.parameter = okParam ? paramVal : 0;
                hotkeysMap[scancode] = data;
            }
        }
    }

    if (xml.hasError()) {
        qWarning() << "Parsing error in hotkeys.xml:" << xml.errorString();
    }

    file.close();
    return hotkeysMap;
}

QVariantMap HotkeysManager::toVariantMap(const QMap<uint32_t, HotkeyData>& data)
{
    QVariantMap result;

    for (auto it = data.begin(); it != data.end(); ++it) {
        QString keyStr = QString::number(it.key());

        QVariantMap hotkeyData;
        hotkeyData["functionName"] = it.value().functionName;
        hotkeyData["parameter"]     = it.value().parameter;

        result[keyStr] = hotkeyData;
    }

    return result;
}

QList<HotkeyData> HotkeysManager::parseHotkeysFromString(const QString &xmlString) const
{
    QList<HotkeyData> result;

    QXmlStreamReader xml(xmlString);
    while (!xml.atEnd() && !xml.hasError()) {
        auto token = xml.readNext();
        if (token == QXmlStreamReader::StartElement && xml.name() == "Hotkey") {
            QXmlStreamAttributes attrs = xml.attributes();

            HotkeyData data;
            data.functionName = attrs.value("functionName").toString();

            bool okScan = false;
            data.scanCode = attrs.value("scanCode").toString().toUInt(&okScan);

            bool okParameter = false;
            data.parameter = attrs.value("parameter").toString().toInt(&okParameter);

            data.description = attrs.value("description").toString();

            if (!data.functionName.isEmpty()) {
                result.append(data);
            }
        }
    }

    if (xml.hasError()) {
        qWarning() << "Parsing error in parseHotkeysFromString:" << xml.errorString();
    }

    return result;
}

QList<HotkeyData> HotkeysManager::parseHotkeysFromFile(const QString &filePath) const
{
    QList<HotkeyData> result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for reading:" << filePath;
        return result;
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd() && !xml.hasError()) {
        auto token = xml.readNext();
        if (token == QXmlStreamReader::StartElement && xml.name() == "Hotkey") {
            QXmlStreamAttributes attrs = xml.attributes();

            HotkeyData data;
            data.functionName = attrs.value("functionName").toString();

            bool okScan = false;
            data.scanCode = attrs.value("scanCode").toString().toUInt(&okScan);

            bool okParameter = false;
            data.parameter = attrs.value("parameter").toString().toInt(&okParameter);

            data.description = attrs.value("description").toString();

            if (!data.functionName.isEmpty()) {
                result.append(data);
            }
        }
    }

    if (xml.hasError()) {
        qWarning() << "Parsing error in parseHotkeysFromFile:" << xml.errorString();
    }

    file.close();
    return result;
}

bool HotkeysManager::saveHotkeysToFile(const QList<HotkeyData> &list, const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for writing:" << filePath;
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("Hotkeys");

    for (const HotkeyData &item : list) {
        xml.writeStartElement("Hotkey");

        xml.writeAttribute("functionName", item.functionName);

        if (item.scanCode != 0) {
            xml.writeAttribute("scanCode", QString::number(item.scanCode));
        }
        if (item.parameter != 0) {
            xml.writeAttribute("parameter", QString::number(item.parameter));
        }
        if (!item.description.isEmpty()) {
            xml.writeAttribute("description", item.description);
        }

        xml.writeEndElement();
    }

    xml.writeEndElement();
    xml.writeEndDocument();

    file.close();
    return true;
}

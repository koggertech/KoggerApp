#include "hotkeys_manager.h"

#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QHash>
#include <QStandardPaths>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>


static const char* hotkeyDescription(const QString& functionName)
{
    // QT_TRANSLATE_NOOP marks strings for lupdate; translation happens via
    // QCoreApplication::translate("HotkeysManager", ...) at call site.
    static const QHash<QString, const char*> map = {
        // Application
        { "closeSettings",      QT_TRANSLATE_NOOP("HotkeysManager", "Close menu") },
        { "clickConnections",   QT_TRANSLATE_NOOP("HotkeysManager", "Connection button") },
        { "clickSettings",      QT_TRANSLATE_NOOP("HotkeysManager", "Settings button") },
        { "click3D",            QT_TRANSLATE_NOOP("HotkeysManager", "3D button") },
        { "click2D",            QT_TRANSLATE_NOOP("HotkeysManager", "2D button") },
        { "toggleFullScreen",   QT_TRANSLATE_NOOP("HotkeysManager", "Toggle fullscreen") },
        { "openFile",           QT_TRANSLATE_NOOP("HotkeysManager", "Open last file") },
        { "openFileDialog",     QT_TRANSLATE_NOOP("HotkeysManager", "Open file dialog") },
        { "closeFile",          QT_TRANSLATE_NOOP("HotkeysManager", "Close file") },
        // Echogram
        { "horScrollRight",     QT_TRANSLATE_NOOP("HotkeysManager", "Scroll the echogram to the left") },
        { "horScrollLeft",      QT_TRANSLATE_NOOP("HotkeysManager", "Scroll the echogram to the right") },
        { "verScrollUp",        QT_TRANSLATE_NOOP("HotkeysManager", "Scroll up the echogram") },
        { "verScrollDown",      QT_TRANSLATE_NOOP("HotkeysManager", "Scroll down the echogram") },
        { "verZoomIn",          QT_TRANSLATE_NOOP("HotkeysManager", "Zoom in vertically on the echogram") },
        { "verZoomOut",         QT_TRANSLATE_NOOP("HotkeysManager", "Zoom out vertically on the echogram") },
        { "increaseLowLevel",   QT_TRANSLATE_NOOP("HotkeysManager", "Increase the lower brightness level of the echogram") },
        { "decreaseLowLevel",   QT_TRANSLATE_NOOP("HotkeysManager", "Decrease the lower brightness level of the echogram") },
        { "increaseHighLevel",  QT_TRANSLATE_NOOP("HotkeysManager", "Increase the upper brightness level of the echogram") },
        { "decreaseHighLevel",  QT_TRANSLATE_NOOP("HotkeysManager", "Decrease the upper brightness level of the echogram") },
        { "prevTheme",          QT_TRANSLATE_NOOP("HotkeysManager", "Switch the echogram theme to the previous one") },
        { "nextTheme",          QT_TRANSLATE_NOOP("HotkeysManager", "Switch the echogram theme to the next one") },
        { "toggleEchogramType", QT_TRANSLATE_NOOP("HotkeysManager", "Toggle echogram type raw/side-scan") },
        // 3D
        { "scene3dZoomIn",        QT_TRANSLATE_NOOP("HotkeysManager", "3D movement along the Z-axis down") },
        { "scene3dZoomOut",       QT_TRANSLATE_NOOP("HotkeysManager", "3D movement along the Z-axis up") },
        { "resetDepthZoom3D",     QT_TRANSLATE_NOOP("HotkeysManager", "Z-axis scaling in 3D: reset") },
        { "resetCameraTop3D",     QT_TRANSLATE_NOOP("HotkeysManager", "3D Camera: Reset Rotation") },
        { "cameraShiftXMinus3D",  QT_TRANSLATE_NOOP("HotkeysManager", "3D camera: Y-axis shift downward") },
        { "cameraShiftXPlus3D",   QT_TRANSLATE_NOOP("HotkeysManager", "3D camera: Y-axis shift upward") },
        { "cameraShiftYMinus3D",  QT_TRANSLATE_NOOP("HotkeysManager", "3D camera: shift to the right along the X-axis") },
        { "cameraShiftYPlus3D",   QT_TRANSLATE_NOOP("HotkeysManager", "3D camera: shift left along the X-axis") },
        { "cameraShiftZMinus3D",  QT_TRANSLATE_NOOP("HotkeysManager", "Scaling along the Z-axis in 3D: decrease") },
        { "cameraShiftZPlus3D",   QT_TRANSLATE_NOOP("HotkeysManager", "Scaling along the Z-axis in 3D: increase") },
        { "toggleBottomTrack3D",  QT_TRANSLATE_NOOP("HotkeysManager", "Toggle 3D bottom track") },
        { "toggleIsobaths3D",     QT_TRANSLATE_NOOP("HotkeysManager", "Toggle 3D isobaths") },
        { "toggleMosaic3D",       QT_TRANSLATE_NOOP("HotkeysManager", "Toggle 3D mosaic") },
        // Mosaic
        { "mosaicPrevTheme",      QT_TRANSLATE_NOOP("HotkeysManager", "Mosaic previous theme") },
        { "mosaicNextTheme",      QT_TRANSLATE_NOOP("HotkeysManager", "Mosaic next theme") },
        { "mosaicLowLevelUp",     QT_TRANSLATE_NOOP("HotkeysManager", "Mosaic lower level up") },
        { "mosaicLowLevelDown",   QT_TRANSLATE_NOOP("HotkeysManager", "Mosaic lower level down") },
        { "mosaicHighLevelUp",    QT_TRANSLATE_NOOP("HotkeysManager", "Mosaic upper level up") },
        { "mosaicHighLevelDown",  QT_TRANSLATE_NOOP("HotkeysManager", "Mosaic upper level down") },
        // Surface
        { "surfacePrevTheme",  QT_TRANSLATE_NOOP("HotkeysManager", "Surface previous theme") },
        { "surfaceNextTheme",  QT_TRANSLATE_NOOP("HotkeysManager", "Surface next theme") },
        { "surfaceStepDown",   QT_TRANSLATE_NOOP("HotkeysManager", "Surface isobaths step down") },
        { "surfaceStepUp",     QT_TRANSLATE_NOOP("HotkeysManager", "Surface isobaths step up") },
    };
    auto it = map.constFind(functionName);
    return it != map.constEnd() ? *it : nullptr;
}


// default hotkeys template
#if defined(Q_OS_LINUX)
const char* HotkeysManager::s_defaultHotkeysXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<Hotkeys>
    <!-- Application -->
    <Hotkey functionName="closeSettings"     scanCode="9"  group="Application"/>
    <Hotkey functionName="clickConnections"  scanCode="56" group="Application"/>
    <Hotkey functionName="clickSettings"     scanCode="57" group="Application"/>
    <Hotkey functionName="click3D"           scanCode="52" group="Application"/>
    <Hotkey functionName="click2D"           scanCode="53" group="Application"/>
    <Hotkey functionName="toggleFullScreen"  scanCode="95" group="Application"/>
    <Hotkey functionName="openFile"          scanCode="76" group="Application"/>
    <Hotkey functionName="openFileDialog"    scanCode="96" group="Application"/>
    <Hotkey functionName="closeFile"         scanCode="75" group="Application"/>
    <!-- Echogram -->
    <Hotkey functionName="horScrollRight"    scanCode="38" parameter="100" group="Echogram"/>
    <Hotkey functionName="horScrollLeft"     scanCode="24" parameter="100" group="Echogram"/>
    <Hotkey functionName="verScrollUp"       scanCode="25" parameter="100" group="Echogram"/>
    <Hotkey functionName="verScrollDown"     scanCode="39" parameter="100" group="Echogram"/>
    <Hotkey functionName="verZoomIn"         scanCode="40" parameter="100" group="Echogram"/>
    <Hotkey functionName="verZoomOut"        scanCode="26" parameter="100" group="Echogram"/>
    <Hotkey functionName="increaseLowLevel"  scanCode="27" parameter="1"   group="Echogram"/>
    <Hotkey functionName="decreaseLowLevel"  scanCode="41" parameter="1"   group="Echogram"/>
    <Hotkey functionName="increaseHighLevel" scanCode="28" parameter="1"   group="Echogram"/>
    <Hotkey functionName="decreaseHighLevel" scanCode="42" parameter="1"   group="Echogram"/>
    <Hotkey functionName="prevTheme"         scanCode="54" group="Echogram"/>
    <Hotkey functionName="nextTheme"         scanCode="55" group="Echogram"/>
    <Hotkey functionName="toggleEchogramType" scanCode="61" group="Echogram"/>
    <!-- 3D -->
    <Hotkey functionName="scene3dZoomIn"       scanCode="29" group="3D"/>
    <Hotkey functionName="scene3dZoomOut"      scanCode="43" group="3D"/>
    <Hotkey functionName="resetDepthZoom3D"    scanCode="17" group="3D"/>
    <Hotkey functionName="resetCameraTop3D"    scanCode="14" group="3D"/>
    <Hotkey functionName="cameraShiftXMinus3D" scanCode="10" group="3D"/>
    <Hotkey functionName="cameraShiftXPlus3D"  scanCode="11" group="3D"/>
    <Hotkey functionName="cameraShiftYMinus3D" scanCode="12" group="3D"/>
    <Hotkey functionName="cameraShiftYPlus3D"  scanCode="13" group="3D"/>
    <Hotkey functionName="cameraShiftZMinus3D" scanCode="15" group="3D"/>
    <Hotkey functionName="cameraShiftZPlus3D"  scanCode="16" group="3D"/>
    <Hotkey functionName="toggleBottomTrack3D" scanCode="50" group="3D"/>
    <Hotkey functionName="toggleIsobaths3D"    scanCode="59" group="3D"/>
    <Hotkey functionName="toggleMosaic3D"      scanCode="60" group="3D"/>
    <!-- Mosaic -->
    <Hotkey functionName="mosaicPrevTheme"     scanCode="30" group="Mosaic"/>
    <Hotkey functionName="mosaicNextTheme"     scanCode="44" group="Mosaic"/>
    <Hotkey functionName="mosaicLowLevelUp"    scanCode="31" parameter="1" group="Mosaic"/>
    <Hotkey functionName="mosaicLowLevelDown"  scanCode="45" parameter="1" group="Mosaic"/>
    <Hotkey functionName="mosaicHighLevelUp"   scanCode="32" parameter="1" group="Mosaic"/>
    <Hotkey functionName="mosaicHighLevelDown" scanCode="46" parameter="1" group="Mosaic"/>
    <!-- Surface -->
    <Hotkey functionName="surfacePrevTheme" scanCode="33" group="Surface"/>
    <Hotkey functionName="surfaceNextTheme" scanCode="47" group="Surface"/>
    <Hotkey functionName="surfaceStepDown"  scanCode="34" parameter="1" group="Surface"/>
    <Hotkey functionName="surfaceStepUp"    scanCode="48" parameter="1" group="Surface"/>
</Hotkeys>
)";
#else
const char* HotkeysManager::s_defaultHotkeysXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<Hotkeys>
    <!-- Application -->
    <Hotkey functionName="closeSettings"     scanCode="1"  group="Application"/>
    <Hotkey functionName="clickConnections"  scanCode="48" group="Application"/>
    <Hotkey functionName="clickSettings"     scanCode="49" group="Application"/>
    <Hotkey functionName="click3D"           scanCode="44" group="Application"/>
    <Hotkey functionName="click2D"           scanCode="45" group="Application"/>
    <Hotkey functionName="toggleFullScreen"  scanCode="87" group="Application"/>
    <Hotkey functionName="openFile"          scanCode="68" group="Application"/>
    <Hotkey functionName="openFileDialog"    scanCode="88" group="Application"/>
    <Hotkey functionName="closeFile"         scanCode="67" group="Application"/>
    <!-- Echogram -->
    <Hotkey functionName="horScrollRight"    scanCode="30" parameter="100" group="Echogram"/>
    <Hotkey functionName="horScrollLeft"     scanCode="16" parameter="100" group="Echogram"/>
    <Hotkey functionName="verScrollUp"       scanCode="17" parameter="100" group="Echogram"/>
    <Hotkey functionName="verScrollDown"     scanCode="31" parameter="100" group="Echogram"/>
    <Hotkey functionName="verZoomIn"         scanCode="32" parameter="100" group="Echogram"/>
    <Hotkey functionName="verZoomOut"        scanCode="18" parameter="100" group="Echogram"/>
    <Hotkey functionName="increaseLowLevel"  scanCode="19" parameter="1"   group="Echogram"/>
    <Hotkey functionName="decreaseLowLevel"  scanCode="33" parameter="1"   group="Echogram"/>
    <Hotkey functionName="increaseHighLevel" scanCode="20" parameter="1"   group="Echogram"/>
    <Hotkey functionName="decreaseHighLevel" scanCode="34" parameter="1"   group="Echogram"/>
    <Hotkey functionName="prevTheme"         scanCode="46" group="Echogram"/>
    <Hotkey functionName="nextTheme"         scanCode="47" group="Echogram"/>
    <Hotkey functionName="toggleEchogramType" scanCode="53" group="Echogram"/>
    <!-- 3D -->
    <Hotkey functionName="scene3dZoomIn"       scanCode="21" group="3D"/>
    <Hotkey functionName="scene3dZoomOut"      scanCode="35" group="3D"/>
    <Hotkey functionName="resetDepthZoom3D"    scanCode="9"  group="3D"/>
    <Hotkey functionName="resetCameraTop3D"    scanCode="6"  group="3D"/>
    <Hotkey functionName="cameraShiftXMinus3D" scanCode="2"  group="3D"/>
    <Hotkey functionName="cameraShiftXPlus3D"  scanCode="3"  group="3D"/>
    <Hotkey functionName="cameraShiftYMinus3D" scanCode="4"  group="3D"/>
    <Hotkey functionName="cameraShiftYPlus3D"  scanCode="5"  group="3D"/>
    <Hotkey functionName="cameraShiftZMinus3D" scanCode="7"  group="3D"/>
    <Hotkey functionName="cameraShiftZPlus3D"  scanCode="8"  group="3D"/>
    <Hotkey functionName="toggleBottomTrack3D" scanCode="50" group="3D"/>
    <Hotkey functionName="toggleIsobaths3D"    scanCode="51" group="3D"/>
    <Hotkey functionName="toggleMosaic3D"      scanCode="52" group="3D"/>
    <!-- Mosaic -->
    <Hotkey functionName="mosaicPrevTheme"     scanCode="22" group="Mosaic"/>
    <Hotkey functionName="mosaicNextTheme"     scanCode="36" group="Mosaic"/>
    <Hotkey functionName="mosaicLowLevelUp"    scanCode="23" parameter="1" group="Mosaic"/>
    <Hotkey functionName="mosaicLowLevelDown"  scanCode="37" parameter="1" group="Mosaic"/>
    <Hotkey functionName="mosaicHighLevelUp"   scanCode="24" parameter="1" group="Mosaic"/>
    <Hotkey functionName="mosaicHighLevelDown" scanCode="38" parameter="1" group="Mosaic"/>
    <!-- Surface -->
    <Hotkey functionName="surfacePrevTheme" scanCode="25" group="Surface"/>
    <Hotkey functionName="surfaceNextTheme" scanCode="39" group="Surface"/>
    <Hotkey functionName="surfaceStepDown"  scanCode="26" parameter="1" group="Surface"/>
    <Hotkey functionName="surfaceStepUp"    scanCode="40" parameter="1" group="Surface"/>
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

        // build lookup map from defaults
        QMap<QString, HotkeyData> defaultByFunction;
        for (const auto& hk : std::as_const(defaultList))
            defaultByFunction[hk.functionName] = hk;

        // remove hotkeys that no longer exist in defaults
        bool wasModified = false;
        for (int i = existingList.size() - 1; i >= 0; --i) {
            if (!defaultByFunction.contains(existingList[i].functionName)) {
                existingList.removeAt(i);
                wasModified = true;
            }
        }

        // add missing hotkeys and sync group/description from defaults
        QSet<QString> existingFunctions;
        for (const auto& hk : std::as_const(existingList))
            existingFunctions.insert(hk.functionName);

        for (auto& hk : existingList) {
            const HotkeyData& def = defaultByFunction[hk.functionName];
            if (hk.group != def.group) {
                hk.group    = def.group;
                wasModified = true;
            }
        }

        for (const auto& hk : std::as_const(defaultList)) {
            if (!existingFunctions.contains(hk.functionName)) {
                existingList.append(hk);
                wasModified = true;
            }
        }

        // Reorder existing entries to match the canonical default order
        {
            QMap<QString, HotkeyData> byFunction;
            for (const auto& hk : std::as_const(existingList))
                byFunction[hk.functionName] = hk;

            QList<HotkeyData> reordered;
            reordered.reserve(existingList.size());
            for (const auto& def : std::as_const(defaultList)) {
                if (byFunction.contains(def.functionName))
                    reordered.append(byFunction[def.functionName]);
            }

            bool orderChanged = (reordered.size() != existingList.size());
            if (!orderChanged) {
                for (int i = 0; i < reordered.size(); ++i) {
                    if (reordered[i].functionName != existingList[i].functionName) {
                        orderChanged = true;
                        break;
                    }
                }
            }
            if (orderChanged) {
                existingList = reordered;
                wasModified = true;
            }
        }

        if (wasModified) {
            if (saveHotkeysToFile(existingList, filePath)) {
                qDebug() << "hotkeys.xml was updated in" << filePath;
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
                data.group     = attrs.value("group").toString();
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

            data.group = attrs.value("group").toString();

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

            data.group = attrs.value("group").toString();

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
        if (!item.group.isEmpty()) {
            xml.writeAttribute("group", item.group);
        }

        xml.writeEndElement();
    }

    xml.writeEndElement();
    xml.writeEndDocument();

    file.close();
    return true;
}

static QString scanCodeToKeyName(uint32_t scanCode)
{
#if defined(Q_OS_LINUX)
    if (scanCode >= 8)
        scanCode -= 8;
#endif
    static const QHash<uint32_t, const char*> keyMap = {
        {1,  "Esc"},       {2,  "1"},         {3,  "2"},         {4,  "3"},
        {5,  "4"},         {6,  "5"},         {7,  "6"},         {8,  "7"},
        {9,  "8"},         {10, "9"},         {11, "0"},         {12, "-"},
        {13, "="},         {14, "Backspace"}, {15, "Tab"},
        {16, "Q"},         {17, "W"},         {18, "E"},         {19, "R"},
        {20, "T"},         {21, "Y"},         {22, "U"},         {23, "I"},
        {24, "O"},         {25, "P"},         {26, "["},         {27, "]"},
        {28, "Enter"},     {29, "Ctrl"},      {30, "A"},         {31, "S"},
        {32, "D"},         {33, "F"},         {34, "G"},         {35, "H"},
        {36, "J"},         {37, "K"},         {38, "L"},         {39, ";"},
        {40, "'"},         {41, "`"},         {42, "LShift"},    {43, "\\"},
        {44, "Z"},         {45, "X"},         {46, "C"},         {47, "V"},
        {48, "B"},         {49, "N"},         {50, "M"},         {51, ","},
        {52, "."},         {53, "/"},         {54, "RShift"},    {55, "Num *"},
        {56, "Alt"},       {57, "Space"},     {58, "CapsLock"},
        {59, "F1"},        {60, "F2"},        {61, "F3"},        {62, "F4"},
        {63, "F5"},        {64, "F6"},        {65, "F7"},        {66, "F8"},
        {67, "F9"},        {68, "F10"},       {87, "F11"},       {88, "F12"},
        {69, "NumLock"},   {70, "ScrollLock"},
        {71, "Num 7"},     {72, "Num 8"},     {73, "Num 9"},     {74, "Num -"},
        {75, "Num 4"},     {76, "Num 5"},     {77, "Num 6"},     {78, "Num +"},
        {79, "Num 1"},     {80, "Num 2"},     {81, "Num 3"},
        {82, "Num 0"},     {83, "Num ."},
    };
    auto it = keyMap.find(scanCode);
    if (it != keyMap.end())
        return QString::fromLatin1(*it);
    return QString("SC:%1").arg(scanCode);
}

QList<HotkeyData> HotkeysManager::loadHotkeysList() const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return parseHotkeysFromFile(configDir + "/hotkeys.xml");
}

QVariantList HotkeysManager::toDisplayVariantList(const QList<HotkeyData>& list)
{
    QVariantList result;
    for (const HotkeyData& hk : list) {
        QVariantMap entry;
        entry["scanCode"]     = hk.scanCode;
        entry["keyName"]      = scanCodeToKeyName(hk.scanCode);
        entry["functionName"] = hk.functionName;
        entry["parameter"]    = hk.parameter;
        entry["group"]        = hk.group;
        const char* desc = hotkeyDescription(hk.functionName);
        entry["description"]  = desc
            ? QCoreApplication::translate("HotkeysManager", desc)
            : hk.functionName;
        result.append(entry);
    }
    return result;
}

bool HotkeysManager::resetToDefaults()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString filePath  = configDir + "/hotkeys.xml";

    QList<HotkeyData> defaults = parseHotkeysFromString(QString::fromUtf8(s_defaultHotkeysXml));
    return saveHotkeysToFile(defaults, filePath);
}

bool HotkeysManager::updateHotkey(const QString& functionName, uint32_t newScanCode, uint32_t parameter)
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString filePath  = configDir + "/hotkeys.xml";

    QList<HotkeyData> list = parseHotkeysFromFile(filePath);

    if (newScanCode == 0)
        return false;

    for (auto& hk : list) {
        if (hk.functionName == functionName) {
            hk.scanCode  = newScanCode;
            if (hk.parameter > 0)
                hk.parameter = qBound(1u, parameter, 99999u);
            return saveHotkeysToFile(list, filePath);
        }
    }
    return false;
}

#include "hotkeys_manager.h"

#include <QFile>
#include <QDir>
#include <QDebug>
#include <QStandardPaths>
#include <QXmlStreamReader>

// default hotkeys template
const char* HotkeysManager::s_defaultHotkeysXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<Hotkeys>
    <Hotkey scanCode="30" functionName="horScrollRight"    step="100" description="horizontal scroll of echogram to the right"/>
    <Hotkey scanCode="16" functionName="horScrollLeft"     step="100" description="horizontal scroll of echogram to the left"/>
    <Hotkey scanCode="17" functionName="verScrollUp"       step="100" description="vertical scroll of echogram up"/>
    <Hotkey scanCode="31" functionName="verScrollDown"     step="100" description="vertical scroll of echogram down"/>
    <Hotkey scanCode="32" functionName="verZoomIn"         step="100" description="echogram vertical zoom in"/>
    <Hotkey scanCode="18" functionName="verZoomOut"        step="100" description="echogram vertical zoom out"/>
    <Hotkey scanCode="19" functionName="increaseLowLevel"  step="1"   description="raise the lower slider of the echogram brightness"/>
    <Hotkey scanCode="33" functionName="decreaseLowLevel"  step="1"   description="lower the lower slider of the echogram brightness"/>
    <Hotkey scanCode="20" functionName="increaseHighLevel" step="1"   description="raise the upper slider of the echogram brightness"/>
    <Hotkey scanCode="34" functionName="decreaseHighLevel" step="1"   description="lower the upper slider of the echogram brightness"/>
    <Hotkey scanCode="44" functionName="click3D"                      description="click 3D button"/>
    <Hotkey scanCode="45" functionName="click2D"                      description="click 2D button"/>
    <Hotkey scanCode="46" functionName="prevTheme"                    description="switch the echogram theme to the previous one"/>
    <Hotkey scanCode="47" functionName="nextTheme"                    description="switch the echogram theme to the next one"/>
    <Hotkey scanCode="87" functionName="toggleFullScreen"             description="toggle fullscreen"/>
</Hotkeys>
)";

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
        //qDebug() << "hotkeys.xml already exists at" << filePath;
    }
}

QMap<quint32, HotkeyData> HotkeysManager::loadHotkeysMapping() const
{
    QMap<quint32, HotkeyData> hotkeysMap;

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
            quint32 scancode = scancodeStr.toUInt(&okSC);

            QString functionName = attrs.value("functionName").toString();

            QString paramStr = attrs.value("step").toString();
            bool okParam = false;
            int paramVal = paramStr.toInt(&okParam);

            if (okSC && scancode != 0 && !functionName.isEmpty()) {
                HotkeyData data;
                data.funcName = functionName;
                data.step = okParam ? paramVal : 0;
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

QVariantMap HotkeysManager::toVariantMap(const QMap<quint32, HotkeyData>& data)
{
    QVariantMap result;

    for (auto it = data.begin(); it != data.end(); ++it) {
        QString keyStr = QString::number(it.key());

        QVariantMap hotkeyData;
        hotkeyData["funcName"] = it.value().funcName;
        hotkeyData["step"]     = it.value().step;

        result[keyStr] = hotkeyData;
    }

    return result;
}

#pragma once

#include <QMap>
#include <QList>
#include <QString>
#include <QVariant>


struct HotkeyData {
    QString functionName;
    uint32_t scanCode{ 0 };
    uint32_t parameter{ 0 };
    QString description;
};

class HotkeysManager
{
public:
    explicit HotkeysManager();

    /*methods*/
    void ensureDefaultHotkeysFile() const;
    QMap<uint32_t, HotkeyData> loadHotkeysMapping() const;
    static QVariantMap toVariantMap(const QMap<quint32, HotkeyData>& data);

private:
    /*methods*/
    QList<HotkeyData> parseHotkeysFromString(const QString& xmlString) const;
    QList<HotkeyData> parseHotkeysFromFile(const QString& filePath) const;
    bool saveHotkeysToFile(const QList<HotkeyData>& list, const QString& filePath) const;

    /*data*/
    static const char* s_defaultHotkeysXml;
};

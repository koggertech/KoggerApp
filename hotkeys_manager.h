#pragma once

#include <QMap>
#include <QString>
#include <QVariant>

struct HotkeyData {
    QString funcName;
    int step = 0;
};

class HotkeysManager
{
public:
    explicit HotkeysManager();

    void ensureDefaultHotkeysFile() const;
    QMap<quint32, HotkeyData> loadHotkeysMapping() const;
    static QVariantMap toVariantMap(const QMap<quint32, HotkeyData>& data);

private:
    static const char* s_defaultHotkeysXml;
};

#pragma once

#include <QObject>
#include <QHash>
#include <QJsonValue>
#include <QPointer>
#include <QVariant>

class LinkManagerWrapper;

class UIStateSerializer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString lastStatus READ lastStatus NOTIFY lastStatusChanged)

public:
    explicit UIStateSerializer(QObject* parent = nullptr);

    Q_INVOKABLE bool exportToJsonFile(const QString& path);
    Q_INVOKABLE bool importFromJsonFile(const QString& path);

    QString lastError() const { return lastError_; }
    QString lastStatus() const { return lastStatus_; }

    void setQmlRootObject(QObject* rootObject);
    void setLinkManagerWrapper(LinkManagerWrapper* linkManagerWrapper);

signals:
    void lastErrorChanged();
    void lastStatusChanged();

private:
    static QString normalizePath(const QString& path);
    static QString majorMinorFromVersion(const QString& version);
    static QString pinnedLinksFilePath();
    static QJsonValue variantToJsonValue(const QVariant& value);
    static QVariant jsonValueToVariant(const QJsonValue& value);
    static bool isSettingsObject(const QObject* object);

    QString currentAppVersion() const;
    QString currentMajorMinorVersion() const;
    int applyImportedSettingsToQml(const QHash<QString, QVariant>& importedValues) const;
    QByteArray loadPinnedLinksXmlDataForExport() const;
    QString userVisiblePinnedLinksWarning(const QByteArray& xmlData,
                                          bool infrastructureUnavailable) const;
    bool reloadPinnedLinksImmediately(const QByteArray& xmlData,
                                      bool allowSerialLinks,
                                      int* skippedSerialLinks,
                                      bool* infrastructureUnavailable,
                                      QString* error) const;

    void setLastError(const QString& errorText);
    void setLastStatus(const QString& statusText);

private:
    QPointer<QObject> qmlRootObject_;
    QPointer<LinkManagerWrapper> linkManagerWrapper_;
    QString lastError_;
    QString lastStatus_;
};

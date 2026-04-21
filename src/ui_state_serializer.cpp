#include "ui_state_serializer.h"
#include "link_manager_wrapper.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaProperty>
#include <QMetaType>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

namespace {

const QString kDumpType = QStringLiteral("KoggerUiStateDump");
const int kSchemaVersion = 1;
const QString kLinksObjectKey = QStringLiteral("links");
const QString kLinksFormatKey = QStringLiteral("format");
const QString kLinksPayloadKey = QStringLiteral("pinnedLinksXmlBase64");
const QString kLinksFormatPinnedXmlBase64 = QStringLiteral("pinned_links_xml_base64_v1");

enum class PathSyntax {
    kRelativeOrEmpty,
    kWindowsAbsolute,
    kUnixAbsolute
};

bool looksLikePinnedLinksXmlPayload(const QByteArray& xmlData)
{
    const QByteArray trimmed = xmlData.trimmed();
    return !trimmed.isEmpty() &&
           !trimmed.contains('\0') &&
           trimmed.startsWith('<');
}

QString normalizeOsFamilyName(const QString& osFamily)
{
    const QString normalized = osFamily.trimmed().toLower();
    if (normalized.startsWith(QStringLiteral("win"))) {
        return QStringLiteral("windows");
    }
    if (normalized == QStringLiteral("linux") ||
        normalized == QStringLiteral("ubuntu") ||
        normalized == QStringLiteral("debian") ||
        normalized == QStringLiteral("fedora") ||
        normalized == QStringLiteral("unix") ||
        normalized == QStringLiteral("posix")) {
        return QStringLiteral("linux");
    }
    if (normalized == QStringLiteral("mac") ||
        normalized == QStringLiteral("macos") ||
        normalized == QStringLiteral("osx") ||
        normalized == QStringLiteral("darwin")) {
        return QStringLiteral("macos");
    }
    if (normalized == QStringLiteral("android")) {
        return QStringLiteral("android");
    }
    return QString();
}

QString normalizePathStyleName(const QString& pathStyle)
{
    const QString normalized = pathStyle.trimmed().toLower();
    if (normalized.startsWith(QStringLiteral("win"))) {
        return QStringLiteral("windows");
    }
    if (normalized == QStringLiteral("unix") ||
        normalized == QStringLiteral("posix") ||
        normalized == QStringLiteral("linux") ||
        normalized == QStringLiteral("macos") ||
        normalized == QStringLiteral("darwin")) {
        return QStringLiteral("unix");
    }
    return QString();
}

QString currentOsFamily()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macos");
#elif defined(Q_OS_ANDROID)
    return QStringLiteral("android");
#else
    return QStringLiteral("linux");
#endif
}

QString currentPathStyle()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows");
#else
    return QStringLiteral("unix");
#endif
}

PathSyntax detectPathSyntax(const QString& rawPath)
{
    const QString path = rawPath.trimmed();
    if (path.isEmpty()) {
        return PathSyntax::kRelativeOrEmpty;
    }

    if (path.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
        const QUrl url(path);
        const QString host = url.host();
        const QString localPath = url.path();

        if (!host.isEmpty()) {
            return PathSyntax::kWindowsAbsolute;
        }

        static const QRegularExpression windowsFileUrlExpression(QStringLiteral(R"(^/[A-Za-z]:/)"));
        if (windowsFileUrlExpression.match(localPath).hasMatch()) {
            return PathSyntax::kWindowsAbsolute;
        }

        if (localPath.startsWith(QStringLiteral("/"))) {
            return PathSyntax::kUnixAbsolute;
        }

        return PathSyntax::kRelativeOrEmpty;
    }

    static const QRegularExpression windowsDriveExpression(QStringLiteral(R"(^[A-Za-z]:[\\/])"));
    if (windowsDriveExpression.match(path).hasMatch() || path.startsWith(QStringLiteral("\\\\"))) {
        return PathSyntax::kWindowsAbsolute;
    }

    if (path.startsWith(QStringLiteral("/"))) {
        return PathSyntax::kUnixAbsolute;
    }

    return PathSyntax::kRelativeOrEmpty;
}

bool isPathStringImportable(const QString& pathValue)
{
    const PathSyntax syntax = detectPathSyntax(pathValue);
    if (syntax == PathSyntax::kRelativeOrEmpty) {
        return true;
    }

#if defined(Q_OS_WIN)
    return syntax == PathSyntax::kWindowsAbsolute;
#else
    return syntax == PathSyntax::kUnixAbsolute;
#endif
}

bool isPathLikeSettingsKey(const QString& key)
{
    const QString lowered = key.toLower();
    return lowered.contains(QStringLiteral("path")) ||
           lowered.contains(QStringLiteral("folder")) ||
           lowered.contains(QStringLiteral("file")) ||
           lowered == QStringLiteral("recentopenedfiles") ||
           lowered == QStringLiteral("savedprofiles");
}

bool filterPathLikeVariant(const QVariant& value, QVariant* filteredValue)
{
    if (!filteredValue) {
        return false;
    }

    const int typeId = value.metaType().id();
    if (typeId == QMetaType::QString) {
        const QString stringValue = value.toString();
        if (!isPathStringImportable(stringValue)) {
            return false;
        }

        *filteredValue = stringValue;
        return true;
    }

    if (typeId == QMetaType::QStringList) {
        const QStringList sourceList = value.toStringList();
        QStringList filteredList;
        filteredList.reserve(sourceList.size());

        for (const QString& item : sourceList) {
            if (isPathStringImportable(item)) {
                filteredList.append(item);
            }
        }

        if (!sourceList.isEmpty() && filteredList.isEmpty()) {
            return false;
        }

        *filteredValue = filteredList;
        return true;
    }

    if (typeId == QMetaType::QVariantList) {
        const QVariantList sourceList = value.toList();
        QVariantList filteredList;
        filteredList.reserve(sourceList.size());

        for (const QVariant& item : sourceList) {
            if (item.metaType().id() == QMetaType::QString) {
                const QString text = item.toString();
                if (isPathStringImportable(text)) {
                    filteredList.append(text);
                }
            }
            else {
                filteredList.append(item);
            }
        }

        if (!sourceList.isEmpty() && filteredList.isEmpty()) {
            return false;
        }

        *filteredValue = filteredList;
        return true;
    }

    *filteredValue = value;
    return true;
}

void collectPathSyntaxStats(const QJsonValue& value, int* windowsCount, int* unixCount)
{
    if (!windowsCount || !unixCount) {
        return;
    }

    if (value.isString()) {
        const PathSyntax syntax = detectPathSyntax(value.toString());
        if (syntax == PathSyntax::kWindowsAbsolute) {
            ++(*windowsCount);
        }
        else if (syntax == PathSyntax::kUnixAbsolute) {
            ++(*unixCount);
        }
        return;
    }

    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        for (const QJsonValue& item : array) {
            collectPathSyntaxStats(item, windowsCount, unixCount);
        }
    }
}

QString detectOsFamilyFromLinksPayload(const QByteArray& xmlData)
{
    if (xmlData.isEmpty()) {
        return QString();
    }

    const QString xmlText = QString::fromUtf8(xmlData);
    const QString loweredXmlText = xmlText.toLower();
    const bool hasUnixSerialSignature = loweredXmlText.contains(QStringLiteral("/dev/tty"));

    static const QRegularExpression windowsSerialExpression(QStringLiteral(R"((^|[^A-Za-z0-9])COM\d+([^A-Za-z0-9]|$))"),
                                                            QRegularExpression::CaseInsensitiveOption);
    const bool hasWindowsSerialSignature = windowsSerialExpression.match(xmlText).hasMatch();

    if (hasUnixSerialSignature && !hasWindowsSerialSignature) {
        return QStringLiteral("linux");
    }
    if (hasWindowsSerialSignature && !hasUnixSerialSignature) {
        return QStringLiteral("windows");
    }
    return QString();
}

QString detectDumpOsFamily(const QJsonObject& appObject,
                           const QJsonObject& settingsObject,
                           const QByteArray& pinnedLinksXmlData)
{
    const QString explicitOsFamily = normalizeOsFamilyName(appObject.value(QStringLiteral("osFamily")).toString());
    if (!explicitOsFamily.isEmpty()) {
        return explicitOsFamily;
    }

    const QString explicitPathStyle = normalizePathStyleName(appObject.value(QStringLiteral("pathStyle")).toString());
    if (explicitPathStyle == QStringLiteral("windows")) {
        return QStringLiteral("windows");
    }
    if (explicitPathStyle == QStringLiteral("unix")) {
        return QStringLiteral("linux");
    }

    int windowsAbsolutePaths = 0;
    int unixAbsolutePaths = 0;
    for (auto it = settingsObject.constBegin(); it != settingsObject.constEnd(); ++it) {
        if (!isPathLikeSettingsKey(it.key())) {
            continue;
        }

        collectPathSyntaxStats(it.value(), &windowsAbsolutePaths, &unixAbsolutePaths);
    }

    if (windowsAbsolutePaths > unixAbsolutePaths && windowsAbsolutePaths > 0) {
        return QStringLiteral("windows");
    }
    if (unixAbsolutePaths > windowsAbsolutePaths && unixAbsolutePaths > 0) {
        return QStringLiteral("linux");
    }

    return detectOsFamilyFromLinksPayload(pinnedLinksXmlData);
}

}

UIStateSerializer::UIStateSerializer(QObject* parent) : QObject(parent)
{
}

bool UIStateSerializer::exportToJsonFile(const QString& path)
{
    setLastError(QString());

    const QString filePath = normalizePath(path);
    if (filePath.isEmpty()) {
        setLastError(tr("Export failed: file path is empty"));
        setLastStatus(QString());
        return false;
    }

    QSettings settings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"));
    const QStringList keys = settings.allKeys();

    QJsonObject settingsObject;
    for (const QString& key : keys) {
        settingsObject.insert(key, variantToJsonValue(settings.value(key)));
    }

    QJsonObject appObject;
    appObject.insert(QStringLiteral("name"), QCoreApplication::applicationName());
    appObject.insert(QStringLiteral("version"), currentAppVersion());
    appObject.insert(QStringLiteral("majorMinor"), currentMajorMinorVersion());
    appObject.insert(QStringLiteral("savedAtUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    appObject.insert(QStringLiteral("osFamily"), currentOsFamily());
    appObject.insert(QStringLiteral("pathStyle"), currentPathStyle());

    QJsonObject rootObject;
    rootObject.insert(QStringLiteral("type"), kDumpType);
    rootObject.insert(QStringLiteral("schemaVersion"), kSchemaVersion);
    rootObject.insert(QStringLiteral("app"), appObject);
    rootObject.insert(QStringLiteral("settings"), settingsObject);

    const QByteArray pinnedLinksXmlData = loadPinnedLinksXmlDataForExport();
    if (!pinnedLinksXmlData.isEmpty()) {
        QJsonObject linksObject;
        linksObject.insert(kLinksFormatKey, kLinksFormatPinnedXmlBase64);
        linksObject.insert(kLinksPayloadKey, QString::fromLatin1(pinnedLinksXmlData.toBase64()));
        rootObject.insert(kLinksObjectKey, linksObject);
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setLastError(tr("Export failed: cannot open file for writing"));
        setLastStatus(QString());
        return false;
    }

    const QByteArray data = QJsonDocument(rootObject).toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size()) {
        setLastError(tr("Export failed: short write"));
        setLastStatus(QString());
        return false;
    }

    setLastStatus(tr("Exported %1 keys.").arg(keys.size()));
    return true;
}

bool UIStateSerializer::importFromJsonFile(const QString& path)
{
    setLastError(QString());

    const QString filePath = normalizePath(path);
    if (filePath.isEmpty()) {
        setLastError(tr("Import failed: file path is empty"));
        setLastStatus(QString());
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setLastError(tr("Import failed: cannot open file"));
        setLastStatus(QString());
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (document.isNull() || parseError.error != QJsonParseError::NoError) {
        setLastError(tr("Import failed: invalid JSON: %1").arg(parseError.errorString()));
        setLastStatus(QString());
        return false;
    }

    if (!document.isObject()) {
        setLastError(tr("Import failed: root must be JSON object"));
        setLastStatus(QString());
        return false;
    }

    const QJsonObject rootObject = document.object();
    const QString type = rootObject.value(QStringLiteral("type")).toString();
    if (type != kDumpType) {
        setLastError(tr("Import failed: unsupported dump type"));
        setLastStatus(QString());
        return false;
    }

    const int schemaVersion = rootObject.value(QStringLiteral("schemaVersion")).toInt(-1);
    if (schemaVersion != kSchemaVersion) {
        setLastError(tr("Import failed: unsupported schema version"));
        setLastStatus(QString());
        return false;
    }

    const QJsonObject appObject = rootObject.value(QStringLiteral("app")).toObject();
    const QString fileVersion = appObject.value(QStringLiteral("version")).toString().trimmed();
    if (fileVersion.isEmpty()) {
        setLastError(tr("Import failed: dump app version is missing"));
        setLastStatus(QString());
        return false;
    }

    const QString currentVersion = currentAppVersion();
    if (fileVersion != currentVersion) {
        setLastError(tr("Import failed: version mismatch (file %1, app %2)")
                         .arg(fileVersion, currentVersion));
        setLastStatus(QString());
        return false;
    }

    const QJsonValue settingsValue = rootObject.value(QStringLiteral("settings"));
    if (!settingsValue.isObject()) {
        setLastError(tr("Import failed: settings must be JSON object"));
        setLastStatus(QString());
        return false;
    }

    const QJsonObject settingsObject = settingsValue.toObject();
    QHash<QString, QVariant> importedValues;
    importedValues.reserve(settingsObject.size());

    bool hasPinnedLinksPayload = false;
    QByteArray pinnedLinksXmlData;
    QString linksWarning;
    if (rootObject.contains(kLinksObjectKey)) {
        const QJsonValue linksValue = rootObject.value(kLinksObjectKey);
        if (!linksValue.isObject()) {
            linksWarning = tr("links must be JSON object");
        }
        else {
            const QJsonObject linksObject = linksValue.toObject();
            const QString linksFormat = linksObject.value(kLinksFormatKey).toString();
            if (linksFormat != kLinksFormatPinnedXmlBase64) {
                linksWarning = tr("unsupported links payload format");
            }
            else {
                const QString payloadText = linksObject.value(kLinksPayloadKey).toString();
                if (payloadText.isEmpty()) {
                    linksWarning = tr("links payload is empty");
                }
                else {
                    const auto decodedPayload = QByteArray::fromBase64Encoding(payloadText.toLatin1());
                    if (!decodedPayload) {
                        linksWarning = tr("links payload is not valid Base64");
                    }
                    else {
                        pinnedLinksXmlData = decodedPayload.decoded;
                        hasPinnedLinksPayload = true;
                    }
                }
            }
        }
    }

    const QString dumpOsFamily = detectDumpOsFamily(appObject, settingsObject, pinnedLinksXmlData);
    const QString normalizedDumpOsFamily = normalizeOsFamilyName(dumpOsFamily);
    const QString normalizedCurrentOsFamily = normalizeOsFamilyName(currentOsFamily());
    const bool isCrossOsImport = !normalizedDumpOsFamily.isEmpty() &&
                                 !normalizedCurrentOsFamily.isEmpty() &&
                                 normalizedDumpOsFamily != normalizedCurrentOsFamily;

    int skippedPathKeys = 0;
    QSettings settings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"));
    for (auto it = settingsObject.constBegin(); it != settingsObject.constEnd(); ++it) {
        const QVariant rawValue = jsonValueToVariant(it.value());

        QVariant filteredValue = rawValue;
        if (isPathLikeSettingsKey(it.key()) && !filterPathLikeVariant(rawValue, &filteredValue)) {
            ++skippedPathKeys;
            continue;
        }

        importedValues.insert(it.key(), filteredValue);
        settings.setValue(it.key(), filteredValue);
    }
    settings.sync();

    if (settings.status() != QSettings::NoError) {
        setLastError(tr("Import failed: cannot write settings"));
        setLastStatus(QString());
        return false;
    }

    int skippedSerialLinks = 0;
    QString linksStatus;
    if (hasPinnedLinksPayload) {
        QString linksError;
        bool linksInfrastructureUnavailable = false;
        if (!reloadPinnedLinksImmediately(pinnedLinksXmlData,
                                          !isCrossOsImport,
                                          &skippedSerialLinks,
                                          &linksInfrastructureUnavailable,
                                          &linksError)) {
            linksWarning = userVisiblePinnedLinksWarning(pinnedLinksXmlData,
                                                         linksInfrastructureUnavailable);
        }
        else {
            linksStatus = tr(" Pinned links were replaced live.");
        }
    }

    if (!linksWarning.isEmpty()) {
        linksStatus = tr(" Pinned links were skipped: %1").arg(linksWarning);
    }

    const int appliedCount = applyImportedSettingsToQml(importedValues);
    setLastStatus(tr("Imported %1 keys. Applied %2 in live UI. Skipped path keys: %3. "
                     "Skipped serial links: %4.%5")
                      .arg(importedValues.size())
                      .arg(appliedCount)
                      .arg(skippedPathKeys)
                      .arg(skippedSerialLinks)
                      .arg(linksStatus));
    return true;
}

void UIStateSerializer::setQmlRootObject(QObject* rootObject)
{
    if (qmlRootObject_ == rootObject) {
        return;
    }

    if (qmlRootObject_) {
        disconnect(qmlRootObject_, nullptr, this, nullptr);
    }

    qmlRootObject_ = rootObject;
    if (qmlRootObject_) {
        connect(qmlRootObject_, &QObject::destroyed, this, [this]() {
            qmlRootObject_ = nullptr;
        });
    }
}

void UIStateSerializer::setLinkManagerWrapper(LinkManagerWrapper* linkManagerWrapper)
{
    linkManagerWrapper_ = linkManagerWrapper;
}

QString UIStateSerializer::normalizePath(const QString& path)
{
    const QString trimmedPath = path.trimmed();
    if (trimmedPath.isEmpty()) {
        return QString();
    }

    const QUrl url(trimmedPath);
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }

    if (trimmedPath.startsWith(QStringLiteral("file://"))) {
        return QUrl(trimmedPath).toLocalFile();
    }

    return trimmedPath;
}

QString UIStateSerializer::majorMinorFromVersion(const QString& version)
{
    const QRegularExpression expression(QStringLiteral(R"((\d+)[\.-](\d+))"));
    const QRegularExpressionMatch match = expression.match(version);
    if (!match.hasMatch()) {
        return QString();
    }

    return match.captured(1) + QStringLiteral(".") + match.captured(2);
}

QString UIStateSerializer::pinnedLinksFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        + QStringLiteral("/pinned_links.xml");
}

QJsonValue UIStateSerializer::variantToJsonValue(const QVariant& value)
{
    QVariant normalizedValue = value;
    if (value.metaType().id() == QMetaType::QByteArray) {
        normalizedValue = QString::fromLatin1(value.toByteArray().toBase64());
    }

    QJsonValue jsonValue = QJsonValue::fromVariant(normalizedValue);
    if (jsonValue.isUndefined()) {
        jsonValue = QJsonValue(normalizedValue.toString());
    }
    return jsonValue;
}

QVariant UIStateSerializer::jsonValueToVariant(const QJsonValue& value)
{
    return value.toVariant();
}

bool UIStateSerializer::isSettingsObject(const QObject* object)
{
    if (!object) {
        return false;
    }

    const QMetaObject* metaObject = object->metaObject();
    if (!metaObject) {
        return false;
    }

    return metaObject->indexOfProperty("category") >= 0;
}

QString UIStateSerializer::currentAppVersion() const
{
    QString version = QGuiApplication::applicationDisplayName().trimmed();
    if (majorMinorFromVersion(version).isEmpty()) {
        version = QCoreApplication::applicationVersion().trimmed();
    }

    if (version.isEmpty()) {
        return QStringLiteral("0.0.0");
    }

    return version;
}

QString UIStateSerializer::currentMajorMinorVersion() const
{
    const QString majorMinorVersion = majorMinorFromVersion(currentAppVersion());
    if (!majorMinorVersion.isEmpty()) {
        return majorMinorVersion;
    }
    return QStringLiteral("0.0");
}

int UIStateSerializer::applyImportedSettingsToQml(const QHash<QString, QVariant>& importedValues) const
{
    if (!qmlRootObject_) {
        return 0;
    }

    int appliedCount = 0;

    QList<QObject*> objects;
    objects.reserve(1 + qmlRootObject_->children().size());
    objects.append(qmlRootObject_.data());

    const auto allChildren = qmlRootObject_->findChildren<QObject*>();
    for (QObject* child : allChildren) {
        objects.append(child);
    }

    for (QObject* object : objects) {
        if (!isSettingsObject(object)) {
            continue;
        }

        const QString category = object->property("category").toString();
        const QMetaObject* metaObject = object->metaObject();
        if (!metaObject) {
            continue;
        }

        for (int i = 0; i < metaObject->propertyCount(); ++i) {
            const QMetaProperty property = metaObject->property(i);
            if (!property.isWritable()) {
                continue;
            }

            const QString propertyName = QString::fromUtf8(property.name());
            if (propertyName.isEmpty() ||
                propertyName == QStringLiteral("objectName") ||
                propertyName == QStringLiteral("category") ||
                propertyName == QStringLiteral("fileName") ||
                propertyName == QStringLiteral("location")) {
                continue;
            }

            const QString settingsKey = category.isEmpty()
                                            ? propertyName
                                            : category + QStringLiteral("/") + propertyName;

            const auto valueIt = importedValues.constFind(settingsKey);
            if (valueIt == importedValues.constEnd()) {
                continue;
            }

            if (object->setProperty(property.name(), valueIt.value())) {
                ++appliedCount;
            }
        }
    }

    return appliedCount;
}

bool UIStateSerializer::reloadPinnedLinksImmediately(const QByteArray& xmlData,
                                                     bool allowSerialLinks,
                                                     int* skippedSerialLinks,
                                                     bool* infrastructureUnavailable,
                                                     QString* error) const
{
    if (!linkManagerWrapper_) {
        if (infrastructureUnavailable) {
            *infrastructureUnavailable = true;
        }
        if (error) {
            *error = tr("link manager is not available");
        }
        return false;
    }

    return linkManagerWrapper_->reloadPinnedLinksFromXmlData(xmlData,
                                                             allowSerialLinks,
                                                             skippedSerialLinks,
                                                             infrastructureUnavailable,
                                                             error);
}

QByteArray UIStateSerializer::loadPinnedLinksXmlDataForExport() const
{
    if (linkManagerWrapper_) {
        const QByteArray liveXmlData = linkManagerWrapper_->exportPinnedLinksToXmlData();
        if (looksLikePinnedLinksXmlPayload(liveXmlData)) {
            return liveXmlData;
        }
    }

    QFile pinnedLinksFile(pinnedLinksFilePath());
    if (!pinnedLinksFile.exists()) {
        return QByteArray();
    }

    if (!pinnedLinksFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QByteArray();
    }

    const QByteArray fileXmlData = pinnedLinksFile.readAll();
    if (!looksLikePinnedLinksXmlPayload(fileXmlData)) {
        return QByteArray();
    }

    return fileXmlData;
}

QString UIStateSerializer::userVisiblePinnedLinksWarning(const QByteArray& xmlData,
                                                        bool infrastructureUnavailable) const
{
    if (infrastructureUnavailable) {
        return tr("link manager is not available");
    }

    const QByteArray trimmedXmlData = xmlData.trimmed();
    if (trimmedXmlData.isEmpty()) {
        return tr("payload is empty");
    }

    if (xmlData.contains('\0')) {
        return tr("payload is not XML text");
    }

    if (!trimmedXmlData.startsWith('<')) {
        return tr("payload is not XML text");
    }

    return tr("invalid XML payload");
}

void UIStateSerializer::setLastError(const QString& errorText)
{
    if (lastError_ == errorText) {
        return;
    }
    lastError_ = errorText;
    emit lastErrorChanged();
}

void UIStateSerializer::setLastStatus(const QString& statusText)
{
    if (lastStatus_ == statusText) {
        return;
    }
    lastStatus_ = statusText;
    emit lastStatusChanged();
}

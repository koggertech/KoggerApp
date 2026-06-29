#include "settings_migration.h"

#include <QSettings>
#include <QStringList>

#include <iterator>

namespace {

constexpr int kCurrentSchemaVersion = 6;

void migrateSettingsGroup(QSettings& settings, const QString& from, const QString& to)
{
    settings.beginGroup(from);
    const QStringList keys = settings.allKeys();
    settings.endGroup();
    if (keys.isEmpty()) {
        return;
    }
    for (const QString& key : keys) {
        settings.setValue(to + QLatin1Char('/') + key, settings.value(from + QLatin1Char('/') + key));
    }
    if ((to + QLatin1Char('/')).startsWith(from + QLatin1Char('/'))) {
        // 'to' nests under 'from' (e.g. scene3d -> scene3d/view): removing the whole
        // 'from' subtree would drop the just-written data, so remove old keys one by one.
        for (const QString& key : keys) {
            settings.remove(from + QLatin1Char('/') + key);
        }
    }
    else {
        settings.remove(from);
    }
}

void migrateToV1(QSettings& settings)
{
    migrateSettingsGroup(settings, QStringLiteral("workspace"),         QStringLiteral("main/workspace"));
    migrateSettingsGroup(settings, QStringLiteral("ui"),                QStringLiteral("main/ui"));
    migrateSettingsGroup(settings, QStringLiteral("csv_export_fields"), QStringLiteral("main/csvExportFields"));
    migrateSettingsGroup(settings, QStringLiteral("scene3d"),           QStringLiteral("scene3d/view"));
    migrateSettingsGroup(settings, QStringLiteral("scene3d_ui"),        QStringLiteral("scene3d/ui"));
    migrateSettingsGroup(settings, QStringLiteral("CameraView"),        QStringLiteral("scene3d/cameraView"));
    migrateSettingsGroup(settings, QStringLiteral("LLARef"),            QStringLiteral("scene3d/llaRef"));
    migrateSettingsGroup(settings, QStringLiteral("Map"),               QStringLiteral("scene3d/map"));
    migrateSettingsGroup(settings, QStringLiteral("echogram_aim"),      QStringLiteral("scene2d/echogramAim"));
    migrateSettingsGroup(settings, QStringLiteral("echogram_loupe"),    QStringLiteral("scene2d/echogramLoupe"));
    migrateSettingsGroup(settings, QStringLiteral("echogram_sync"),     QStringLiteral("scene2d/echogramSync"));
    for (int i = 1; i <= 6; ++i) {
        const QString suffix = QString::number(i);
        migrateSettingsGroup(settings, QStringLiteral("Plot2D_") + suffix, QStringLiteral("scene2d/plot2d/") + suffix);
    }

    if (settings.contains(QStringLiteral("appLanguage"))) {
        settings.setValue(QStringLiteral("main/appLanguage"), settings.value(QStringLiteral("appLanguage")));
        settings.remove(QStringLiteral("appLanguage"));
    }
    settings.remove(QStringLiteral("numPlotsSpinBox"));
    settings.remove(QStringLiteral("plotSyncCheckBox"));
}

struct PrefixRule {
    QLatin1StringView prefix;
    QLatin1StringView dest;
};

void migrateRootKeysByPrefix(QSettings& settings, const PrefixRule* rules, int ruleCount)
{
    const QStringList rootKeys = settings.childKeys();
    for (const QString& key : rootKeys) {
        if (key == QLatin1StringView("schemaVersion")) {
            continue;
        }
        for (int i = 0; i < ruleCount; ++i) {
            if (key.startsWith(rules[i].prefix)) {
                const QString dest = rules[i].dest;
                settings.setValue(dest + QLatin1Char('/') + key, settings.value(key));
                settings.remove(key);
                break;
            }
        }
    }
}

void migrateToV2(QSettings& settings)
{
    static constexpr PrefixRule kRules[] = {
        { QLatin1StringView("compass"),         QLatin1StringView("scene3d/compass") },
        { QLatin1StringView("circleGrid"),      QLatin1StringView("scene3d/grid") },
        { QLatin1StringView("navigationArrow"), QLatin1StringView("scene3d/navigationArrow") },
        { QLatin1StringView("mosaic"),          QLatin1StringView("scene3d/mosaic") },
        { QLatin1StringView("isobaths"),        QLatin1StringView("scene3d/isobaths") },
        { QLatin1StringView("geoJson"),         QLatin1StringView("scene3d/view") },
    };
    migrateRootKeysByPrefix(settings, kRules, int(std::size(kRules)));

    settings.remove(QStringLiteral("navigationViewButton"));
    settings.remove(QStringLiteral("useAngleButton"));
}

void migrateToV3(QSettings& settings)
{
    static constexpr PrefixRule kRules[] = {
        { QLatin1StringView("bottomTrack"), QLatin1StringView("scene2d/bottomTrack") },
        { QLatin1StringView("syncLoupe"),   QLatin1StringView("scene2d/echogramLoupe") },
        { QLatin1StringView("fakeCoords"),  QLatin1StringView("scene2d/fakeCoords") },
    };
    migrateRootKeysByPrefix(settings, kRules, int(std::size(kRules)));
}

void moveSettingsKey(QSettings& settings, const QString& from, const QString& to)
{
    if (settings.contains(from)) {
        settings.setValue(to, settings.value(from));
        settings.remove(from);
    }
}

void migrateToV4(QSettings& settings)
{
    static constexpr QLatin1StringView kDeadKeys[] = {
        QLatin1StringView("factory_pn"), QLatin1StringView("factory_sn"), QLatin1StringView("factoryFolder"),
        QLatin1StringView("upgradeFolder"), QLatin1StringView("importFolder"),
        QLatin1StringView("visible2DButtonChecked"), QLatin1StringView("visible3DButtonChecked"),
        QLatin1StringView("tgcGainNearSlider"), QLatin1StringView("tgcGainFarSlider"), QLatin1StringView("tgcCompensateCheck"),
        QLatin1StringView("sonarOffsetValueZ"), QLatin1StringView("updateBottomTrackRealtimeButton"),
        QLatin1StringView("uiStateExportPathSource"), QLatin1StringView("uiStateImportPathSource"),
        QLatin1StringView("importGeoJsonFolder"), QLatin1StringView("exportGeoJsonFolder"),
        QLatin1StringView("profilesButtonCheck"), QLatin1StringView("autopilotInfoVisible"),
        QLatin1StringView("extraBoatInfoVisible"), QLatin1StringView("extraInfoDepthVisible"),
        QLatin1StringView("extraInfoSpeedVisible"), QLatin1StringView("extraInfoCoordinatesVisible"),
        QLatin1StringView("extraInfoActivePointVisible"), QLatin1StringView("extraInfoSimpleNavV2Visible"),
        QLatin1StringView("extraInfoBoatStatusVisible"),
    };
    for (const QLatin1StringView& key : kDeadKeys) {
        settings.remove(key);
    }

    // 3D visibility toggles that earlier prefix steps placed next to group params -> scene3d/view
    moveSettingsKey(settings, QStringLiteral("scene3d/compass/compassCheckButton"),                 QStringLiteral("scene3d/view/compassCheckButton"));
    moveSettingsKey(settings, QStringLiteral("scene3d/navigationArrow/navigationArrowCheckButton"), QStringLiteral("scene3d/view/navigationArrowCheckButton"));
    moveSettingsKey(settings, QStringLiteral("scene2d/echogramLoupe/syncLoupeCheckButton"),         QStringLiteral("scene3d/view/syncLoupeCheckButton"));
    moveSettingsKey(settings, QStringLiteral("scene2d/bottomTrack/bottomTrackCheckButton"),         QStringLiteral("scene3d/view/bottomTrackCheckButton"));
    moveSettingsKey(settings, QStringLiteral("scene3d/isobaths/isobathsCheckButton"),               QStringLiteral("scene3d/view/isobathsCheckButton"));
    moveSettingsKey(settings, QStringLiteral("scene3d/mosaic/mosaicViewCheckButton"),               QStringLiteral("scene3d/view/mosaicViewCheckButton"));

    static constexpr PrefixRule kRules[] = {
        { QLatin1StringView("forceSingleZoom"),   QLatin1StringView("scene3d/view") },
        { QLatin1StringView("isNorthView"),       QLatin1StringView("scene3d/view") },
        { QLatin1StringView("selectionTool"),     QLatin1StringView("scene3d/view") },
        { QLatin1StringView("grid"),              QLatin1StringView("scene3d/view") },
        { QLatin1StringView("shadowEnabled"),     QLatin1StringView("scene3d/view") },
        { QLatin1StringView("scaleBar"),          QLatin1StringView("scene3d/view") },
        { QLatin1StringView("boatTrack"),         QLatin1StringView("scene3d/view") },
        { QLatin1StringView("verticalScale"),     QLatin1StringView("scene3d/view") },
        { QLatin1StringView("mapView"),           QLatin1StringView("scene3d/map") },
        { QLatin1StringView("deferTiles"),        QLatin1StringView("scene3d/map") },
        { QLatin1StringView("extraWidth"),        QLatin1StringView("scene3d/isobaths") },
        { QLatin1StringView("hideEmpty"),         QLatin1StringView("scene2d/ui") },
        { QLatin1StringView("console"),           QLatin1StringView("main/console") },
        { QLatin1StringView("consScroll"),        QLatin1StringView("main/console") },
        { QLatin1StringView("protoBin"),          QLatin1StringView("main/console") },
        { QLatin1StringView("appTheme"),          QLatin1StringView("main/ui") },
        { QLatin1StringView("instrumentsGrade"),  QLatin1StringView("main/ui") },
        { QLatin1StringView("welcomeShown"),      QLatin1StringView("main/ui") },
        { QLatin1StringView("appDoubleTap"),      QLatin1StringView("main/ui") },
        { QLatin1StringView("appSplit"),          QLatin1StringView("main/ui") },
        { QLatin1StringView("appSidebar"),        QLatin1StringView("main/ui") },
        { QLatin1StringView("appWorkspace"),      QLatin1StringView("main/ui") },
        { QLatin1StringView("logging"),           QLatin1StringView("main/logging") },
        { QLatin1StringView("appTgc"),            QLatin1StringView("main/tgc") },
        { QLatin1StringView("fixBlackStripes"),   QLatin1StringView("main/blackStripes") },
        { QLatin1StringView("sonarOffset"),       QLatin1StringView("main/sonarOffset") },
        { QLatin1StringView("zeroing"),           QLatin1StringView("main/sonarOffset") },
        { QLatin1StringView("export"),            QLatin1StringView("main/export") },
        { QLatin1StringView("dev"),               QLatin1StringView("main/devices") },
        { QLatin1StringView("importTrack"),       QLatin1StringView("main/csvImport") },
        { QLatin1StringView("importPath"),        QLatin1StringView("main/csvImport") },
        { QLatin1StringView("importCSV"),         QLatin1StringView("main/csvImport") },
        { QLatin1StringView("separatorCombo"),    QLatin1StringView("main/csvImport") },
        { QLatin1StringView("utcGpsCombo"),       QLatin1StringView("main/csvImport") },
        { QLatin1StringView("flasherPartNumber"), QLatin1StringView("main/csvImport") },
        { QLatin1StringView("logFolder"),         QLatin1StringView("main/files") },
        { QLatin1StringView("recentOpenedFiles"), QLatin1StringView("main/files") },
        { QLatin1StringView("pathText"),          QLatin1StringView("main/files") },
        { QLatin1StringView("profiles"),          QLatin1StringView("main/profiles") },
        { QLatin1StringView("uiState"),           QLatin1StringView("main/uiState") },
    };
    migrateRootKeysByPrefix(settings, kRules, int(std::size(kRules)));
}

void migrateToV6(QSettings& settings)
{
    // fakeCoords + zeroing are dataset prefs, not echogram/sonar -> main/dataset.
    migrateSettingsGroup(settings, QStringLiteral("scene2d/fakeCoords"), QStringLiteral("main/dataset"));
    moveSettingsKey(settings, QStringLiteral("main/sonarOffset/zeroingPosButtonCheched"),         QStringLiteral("main/dataset/zeroingPosButtonCheched"));
    moveSettingsKey(settings, QStringLiteral("main/sonarOffset/zeroingBottomTrackButtonChecked"), QStringLiteral("main/dataset/zeroingBottomTrackButtonChecked"));
}

void migrateToV5(QSettings& settings)
{
    // Final sweep: every live QML Settings now carries a category, so the root
    // should no longer gain loose keys. Whatever still sits at the top level is
    // legacy cruft from removed code — drop it, keeping only the schema marker.
    const QStringList rootKeys = settings.childKeys();
    for (const QString& key : rootKeys) {
        if (key != QLatin1StringView("schemaVersion")) {
            settings.remove(key);
        }
    }
}

}

void migrateSettingsSchema()
{
    QSettings settings;
    const int version = settings.value(QStringLiteral("schemaVersion"), 0).toInt();
    if (version >= kCurrentSchemaVersion) {
        return;
    }

    if (version < 1) {
        migrateToV1(settings);
    }
    if (version < 2) {
        migrateToV2(settings);
    }
    if (version < 3) {
        migrateToV3(settings);
    }
    if (version < 4) {
        migrateToV4(settings);
    }
    if (version < 5) {
        migrateToV5(settings);
    }
    if (version < 6) {
        migrateToV6(settings);
    }

    settings.setValue(QStringLiteral("schemaVersion"), kCurrentSchemaVersion);
    settings.sync();
}

#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QTranslator>
#include <QLocale>
#include <QSettings>
#include <QVector>
#include <QString>
#include <QThread>
#include <QResource>
#include <QFile>
#include <QByteArray>
#include <QQuickWindow>
#include <QPointer>
#include <QSql>
#include <QSqlDatabase>
#include <QQuickStyle>
#include <QWindow>
#include <QStyleHints>
#include <QLoggingCategory>
#include <QHostAddress>
#include <QDebug>
#include <algorithm>
#if defined(Q_OS_WIN)
#include <windows.h>
#endif
#include "qPlot2D.h"
#include "core.h"
#include "themes.h"
#include "ui_probe.h"
#include "ui_state_serializer.h"
#include "echogram_state_serializer.h"
#include "notifications.h"
#include "scene_object.h"
#include "bottom_track.h"
#include "input_device_tracker.h"
#ifndef Q_OS_ANDROID
#include "instance_lock.h"
#endif
#include "system_battery.h"
#include "mosaic_db.h"
#include "language_controller.h"
#include "app_utils.h"
#include "app_log.h"
#include "settings_migration.h"
#include "video_stream_pool.h"
#include "link_discovery.h"
#include "control_server.h"


// NOLINTBEGIN(bugprone-throwing-static-initialization): application-lifetime singletons; a throw here is a fatal startup failure with nothing to catch
Core core;
AppUtils appUtils;
Themes theme;
UIStateSerializer uiStateSerializer;
EchogramStateSerializer echogramStateSerializer;
Notifications notifications;
VideoStreamPool videoStreams;
QTranslator translator;
QVector<QString> availableLanguages{"en", "ru", "pl"};

#ifndef Q_OS_ANDROID
InstanceLock instanceLock;
#endif
// NOLINTEND(bugprone-throwing-static-initialization)


void loadLanguage(QGuiApplication &app)
{
    QSettings settings;
    QString currentLanguage;

    int savedLanguageIndex = settings.value("main/appLanguage", -1).toInt();

    if (savedLanguageIndex == -1) {
        currentLanguage = QLocale::system().name().split('_').first();
        if (auto indx = availableLanguages.indexOf(currentLanguage); indx == -1) {
            currentLanguage = availableLanguages.front();
        }
        else {
            settings.setValue("main/appLanguage", indx);
        }
    }
    else {
        if (savedLanguageIndex >= 0 && savedLanguageIndex < availableLanguages.count()) {
            currentLanguage = availableLanguages.at(savedLanguageIndex);
        }
        else {
            currentLanguage = availableLanguages.front();
        }
    }


    QString translationFile = ":/translations/translation_" + currentLanguage + ".qm";

    if (translator.load(translationFile)) {
        app.installTranslator(&translator);
    }
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);
    core.consoleInfo(msg);
}


QtMessageHandler previousMessageHandler = nullptr;

static bool isVideoLogMessage(const QMessageLogContext& context, const QString& msg)
{
    if (context.category && QByteArray(context.category).startsWith("qt.multimedia")) {
        return true;
    }
    return msg.startsWith(QStringLiteral("VIDEO:"));
}

void videoLogHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    static thread_local bool forwarding = false;

    if (!isVideoLogMessage(context, msg)) {
        AppLog::instance().write(type, context, msg);
    }

    if (!forwarding && isVideoLogMessage(context, msg)) {
        forwarding = true;
        const QString line = msg.startsWith(QStringLiteral("VIDEO:"))
                                 ? msg
                                 : QStringLiteral("VIDEO: ") + msg;
        if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) {
            core.consoleWarning(line);
        }
        else {
            core.consoleInfo(line);
        }
        forwarding = false;
    }

    if (previousMessageHandler && !isVideoLogMessage(context, msg)) {
        previousMessageHandler(type, context, msg);
    }
}

void setApplicationDisplayName(QGuiApplication& app)
{
    QResource resource(":/version.txt");
    if (resource.isValid()) {
        QFile file(":/version.txt");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray data = file.readAll();
            app.setApplicationDisplayName(QString::fromUtf8(data));
            file.close();
        }
    }
}

void registerQmlMetaTypes()
{
    qmlRegisterType<GraphicsScene3dView>("SceneGraphRendering", 1, 0,"GraphicsScene3dView");
    qmlRegisterType<qPlot2D>( "WaterFall", 1, 0, "WaterFall");
    qmlRegisterType<BottomTrack>("BottomTrack", 1, 0, "BottomTrack");
    qRegisterMetaType<BottomTrack::ActionEvent>("BottomTrack::ActionEvent");
    qRegisterMetaType<LinkAttribute>("LinkAttribute");
}

#if defined(Q_OS_WIN)
constexpr DWORD kDwmwaUseImmersiveDarkMode = 20;
constexpr DWORD kDwmwaUseImmersiveDarkModeLegacy = 19;
constexpr DWORD kDwmwaCaptionColor = 35;
constexpr DWORD kDwmwaTextColor = 36;

void applyWindowsSystemTitleBarTheme(QWindow* window)
{
    if (!window) {
        return;
    }

    const HWND handle = reinterpret_cast<HWND>(window->winId()); // NOLINT(performance-no-int-to-ptr): WId is integer, HWND is a pointer; Win32 interop requires the cast
    if (!handle) {
        return;
    }

    const HMODULE dwmApi = LoadLibraryW(L"dwmapi.dll");
    if (!dwmApi) {
        return;
    }

    using DwmSetWindowAttributeFn = HRESULT (WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
    auto* setWindowAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(GetProcAddress(dwmApi, "DwmSetWindowAttribute"));
    if (!setWindowAttribute) {
        FreeLibrary(dwmApi);
        return;
    }

    const QColor captionColor = theme.controlBackColor().darker(108);
    const QColor captionTextColor = theme.textColor();
    const qreal captionLuminance = captionColor.redF() * 0.299 + captionColor.greenF() * 0.587 + captionColor.blueF() * 0.114;

    const BOOL useDarkCaption = captionLuminance < 0.5 ? TRUE : FALSE;
    HRESULT hr = setWindowAttribute(handle,
                                    kDwmwaUseImmersiveDarkMode,
                                    &useDarkCaption,
                                    sizeof(useDarkCaption));
    if (FAILED(hr)) {
        setWindowAttribute(handle,
                           kDwmwaUseImmersiveDarkModeLegacy,
                           &useDarkCaption,
                           sizeof(useDarkCaption));
    }

    const COLORREF captionRef = RGB(captionColor.red(), captionColor.green(), captionColor.blue());
    setWindowAttribute(handle, kDwmwaCaptionColor, &captionRef, sizeof(captionRef));

    const COLORREF captionTextRef = RGB(captionTextColor.red(), captionTextColor.green(), captionTextColor.blue());
    setWindowAttribute(handle, kDwmwaTextColor, &captionTextRef, sizeof(captionTextRef));

    FreeLibrary(dwmApi);
}

void applyWindowsFullscreenBorderWorkaround(QWindow* window)
{
    if (!window) {
        return;
    }

    auto applyBorder = [window]() {
        HWND handle = reinterpret_cast<HWND>(window->winId()); // NOLINT(performance-no-int-to-ptr): WId is integer, HWND is a pointer; Win32 interop requires the cast
        if (!handle) {
            return;
        }

        const LONG_PTR style = GetWindowLongPtr(handle, GWL_STYLE);
        if ((style & WS_BORDER) == 0) {
            SetWindowLongPtr(handle, GWL_STYLE, style | WS_BORDER);
            SetWindowPos(handle, nullptr, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }
    };

    QObject::connect(window, &QWindow::visibilityChanged, window, [applyBorder](QWindow::Visibility visibility) {
        if (visibility == QWindow::FullScreen) {
            applyBorder();
        }
    });

    applyBorder();
}

void bringWindowToFront(QWindow* window)
{
    if (!window || !instanceLock.isPrimary()) {
        return;
    }

    window->raise();
    window->requestActivate();

    const HWND handle = reinterpret_cast<HWND>(window->winId()); // NOLINT(performance-no-int-to-ptr): WId is integer, HWND is a pointer; Win32 interop requires the cast
    if (!handle) {
        return;
    }

    if (IsIconic(handle)) {
        ShowWindow(handle, SW_RESTORE);
    }

    const HWND foreground = GetForegroundWindow();
    const DWORD foregroundThread = foreground ? GetWindowThreadProcessId(foreground, nullptr) : 0;
    const DWORD thisThread = GetCurrentThreadId();
    const bool attach = foregroundThread && foregroundThread != thisThread;

    DWORD savedLockTimeout = 0;
    SystemParametersInfoW(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &savedLockTimeout, 0);
    SystemParametersInfoW(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, reinterpret_cast<PVOID>(static_cast<UINT_PTR>(0)), SPIF_SENDCHANGE); // NOLINT(performance-no-int-to-ptr): Win32 passes an integer value through the pvParam pointer

    if (attach) {
        AttachThreadInput(foregroundThread, thisThread, TRUE);
    }
    AllowSetForegroundWindow(ASFW_ANY);
    SetForegroundWindow(handle);
    BringWindowToTop(handle);
    if (attach) {
        AttachThreadInput(foregroundThread, thisThread, FALSE);
    }

    SystemParametersInfoW(SPI_SETFOREGROUNDLOCKTIMEOUT, 0,
                          reinterpret_cast<PVOID>(static_cast<UINT_PTR>(savedLockTimeout)), SPIF_SENDCHANGE); // NOLINT(performance-no-int-to-ptr): Win32 passes an integer value through the pvParam pointer

    // flash taskbar button
    FLASHWINFO flash = {};
    flash.cbSize = sizeof(flash);
    flash.hwnd = handle;
    flash.dwFlags = FLASHW_TRAY | FLASHW_TIMERNOFG;
    FlashWindowEx(&flash);
}
#endif


struct ControlEndpoint {
    QHostAddress address = QHostAddress::LocalHost;
    quint16      port = 0;
    QString      token;
    QString      artifactRoot;
};

static bool resolveControlEndpoint(const QStringList& args, int instanceIndex, ControlEndpoint& out)
{
    static const QString kTcpFlag       = QStringLiteral("--control-tcp");
    static const QString kTokenFlag     = QStringLiteral("--control-token=");
    static const QString kArtifactsFlag = QStringLiteral("--control-artifacts=");
    static const QStringList kOff       = {QStringLiteral("0"), QStringLiteral("false"), QStringLiteral("off"), QStringLiteral("no")};

    QString spec;
    bool enabled = false;
    for (const QString& a : args) {
        if (a == kTcpFlag) {
            enabled = true;
        } else if (a.startsWith(kTcpFlag + QLatin1Char('='))) {
            enabled = true;
            spec = a.mid(kTcpFlag.size() + 1);
        } else if (a.startsWith(kTokenFlag)) {
            out.token = a.mid(kTokenFlag.size());
        } else if (a.startsWith(kArtifactsFlag)) {
            out.artifactRoot = a.mid(kArtifactsFlag.size());
        }
    }
    if (!enabled) {
        const QString env = qEnvironmentVariable("KOGGER_CONTROL_TCP");
        if (!env.isEmpty() && !kOff.contains(env, Qt::CaseInsensitive)) {
            enabled = true;
            if (env != QLatin1String("1")) {
                spec = env;
            }
        }
    }
    if (out.token.isEmpty()) {
        out.token = qEnvironmentVariable("KOGGER_CONTROL_TOKEN");
    }
    if (out.artifactRoot.isEmpty()) {
        out.artifactRoot = qEnvironmentVariable("KOGGER_CONTROL_ARTIFACTS");
    }
    if (!enabled || kOff.contains(spec, Qt::CaseInsensitive)) {
        return false;
    }

    out.port = static_cast<quint16>(ControlServer::kBasePort + instanceIndex);
    if (spec.isEmpty()) {
        return true;
    }

    QString host;
    QString portStr;
    const bool digitsOnly = std::all_of(spec.cbegin(), spec.cend(), [](QChar c) { return c.isDigit(); });
    if (digitsOnly) {
        portStr = spec;
    } else if (spec.count(QLatin1Char(':')) > 1 && !spec.startsWith(QLatin1Char('['))) {
        host = spec;
    } else if (spec.startsWith(QLatin1Char('['))) {
        const int close = spec.indexOf(QLatin1Char(']'));
        if (close < 0) {
            qWarning().noquote() << QStringLiteral("control: malformed endpoint \"%1\"").arg(spec);
            return false;
        }
        host = spec.mid(1, close - 1);
        if (spec.size() > close + 1 && spec.at(close + 1) == QLatin1Char(':')) {
            portStr = spec.mid(close + 2);
        }
    } else {
        const int colon = spec.lastIndexOf(QLatin1Char(':'));
        host = colon >= 0 ? spec.left(colon) : spec;
        portStr = colon >= 0 ? spec.mid(colon + 1) : QString();
    }

    if (!host.isEmpty()) {
        const QHostAddress addr(host);
        if (addr.isNull()) {
            qWarning().noquote() << QStringLiteral("control: bad host \"%1\"").arg(host);
            return false;
        }
        out.address = addr;
    }
    if (!portStr.isEmpty()) {
        bool ok = false;
        const uint p = portStr.toUInt(&ok);
        if (!ok || p == 0 || p > 65535) {
            qWarning().noquote() << QStringLiteral("control: bad port \"%1\"").arg(portStr);
            return false;
        }
        out.port = static_cast<quint16>(p);
    }
    return true;
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_ANDROID
    // Disable Qt's automatic per-screen scaling: we drive our own DPI-aware
    // UI sizing via Themes::resCoeff (see themes.h). QT_SCALE_FACTOR=0.5
    // halves Qt's internal coordinate system so a high-density tablet
    // doesn't render at the device's full pixel grid (physical px is what
    // we then scale up via resCoeff = physicalDPI / logicalDPI). Net effect
    // on a typical tablet (~2× density): UI sizes match the Desktop 100%
    // baseline at manualScale=1.0.
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0");
    qputenv("QT_SCALE_FACTOR", "0.5");
#endif

#if defined(Q_OS_LINUX)
    QCoreApplication::setAttribute(Qt::AA_ForceRasterWidgets, false);
    ::qputenv("QT_SUPPORT_GL_CHILD_WIDGETS", "1");
#ifdef LINUX_ES
    ::qputenv("QT_OPENGL", "es2");
#endif
#endif

    QCoreApplication::setOrganizationName("KOGGER");
    QCoreApplication::setOrganizationDomain("kogger.tech");
    QCoreApplication::setApplicationName("KoggerApp");
    QCoreApplication::setApplicationVersion("1-1-1");

    migrateSettingsSchema();

#if defined(Q_OS_WIN)
    //QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);
#endif

    QString loggingRules;
#if defined(Q_OS_WIN)
    loggingRules += QStringLiteral("qt.network.info.netlistmanager.warning=false\n"
                                   "qt.qpa.mime=false\n");
#endif
    QLoggingCategory::setFilterRules(loggingRules);

#if defined(Q_OS_ANDROID)
    AppLog::instance().start(AppLog::fallbackDirectory(), QStringLiteral("kogger"), 4LL * 1024 * 1024, 3);
#else
    AppLog::instance().start(AppLog::defaultDirectory(), QStringLiteral("kogger"), 8LL * 1024 * 1024, 5);
#endif

    previousMessageHandler = qInstallMessageHandler(videoLogHandler);

    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);

    QSurfaceFormat format;
#if defined(Q_OS_ANDROID) || defined(LINUX_ES)
    format.setRenderableType(QSurfaceFormat::OpenGLES);
#else
    format.setRenderableType(QSurfaceFormat::OpenGL);
#endif
    format.setSwapInterval(0);

    QSurfaceFormat::setDefaultFormat(format);

    QGuiApplication app(argc, argv);

    app.styleHints()->setMouseDoubleClickInterval(320);

    // Themes global was constructed before QGuiApplication + org name — now
    // safe to read QSettings and primaryScreen() for DPI-aware resCoeff.
    theme.initSettings();

    QQuickStyle::setStyle("Basic");

#ifndef Q_OS_ANDROID
    instanceLock.acquire();
    appUtils.setInstanceIndex(instanceLock.index());
    MosaicDB::setInstanceIndex(instanceLock.index());
#endif

    LanguageController langController;
    InputDeviceTracker inputDeviceTracker;
    SystemBattery systemBattery;
    core.initAfterApp();
    LinkDiscovery linkDiscovery(core.getLinkManagerWrapperPtr());

    //qDebug() << "Lib paths:" << QCoreApplication::libraryPaths();
    //qDebug() << "SQL drivers:" << QSqlDatabase::drivers();

    QCoreApplication::addLibraryPath(QStringLiteral("assets:/qt/plugins"));
    QCoreApplication::addLibraryPath(QStringLiteral(":/android_rcc_bundle/plugins"));
    //qputenv("QT_DEBUG_PLUGINS", "1");
    //qDebug() << "libraryPaths =" << QCoreApplication::libraryPaths();
    loadLanguage(app);
    langController.setStartupTranslator(&translator);
    core.initStreamList();

    setApplicationDisplayName(app);
    QQmlApplicationEngine engine;
    engine.addImportPath("qrc:/");
    engine.addImportPath("qrc:/qml");

    SceneObject::qmlDeclare();

    //qInstallMessageHandler(messageHandler); // TODO: comment this

    registerQmlMetaTypes();

    engine.rootContext()->setContextProperty("dataset", core.getDatasetPtr());
    engine.rootContext()->setContextProperty("core", &core);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("linkManagerWrapper", core.getLinkManagerWrapperPtr());
    engine.rootContext()->setContextProperty("deviceManagerWrapper", core.getDeviceManagerWrapperPtr());
    engine.rootContext()->setContextProperty("deviceTopology", core.getDeviceTopologyModelPtr());
    videoStreams.setSourceModel(core.getLinkManagerWrapperPtr()->getModelPtr());
    QObject::connect(&videoStreams, &VideoStreamPool::streamingChanged,
                     core.getLinkManagerWrapperPtr(), &LinkManagerWrapper::setVideoStreaming);
    engine.rootContext()->setContextProperty("videoStreams", &videoStreams);
    engine.rootContext()->setContextProperty("linkDiscovery", &linkDiscovery);
    engine.rootContext()->setContextProperty("logViewer", core.getConsolePtr());
    engine.rootContext()->setContextProperty("uiStateSerializer", &uiStateSerializer);
    engine.rootContext()->setContextProperty("echogramStateSerializer", &echogramStateSerializer);
    engine.rootContext()->setContextProperty("notifications", &notifications);
    engine.rootContext()->setContextProperty("inputDeviceTracker", &inputDeviceTracker);
    engine.rootContext()->setContextProperty("systemBattery", &systemBattery);
    engine.rootContext()->setContextProperty("langController", &langController);
    engine.rootContext()->setContextProperty("appUtils", &appUtils);

    // Machine-readable UI verification. Costs nothing unless KOGGER_UI_PROBE names an
    // output directory; exposed to QML so an interaction test can dump at a chosen
    // moment instead of on a timer.
    UiProbe uiProbe;
    engine.rootContext()->setContextProperty("uiProbe", &uiProbe);

    ControlServer controlServer;

    // Expose compile-time MANUAL_TESTING flag to QML — the Settings panel
    // shows a "Test" group (with developer-only knobs) only when this is true.
#ifdef MANUAL_TESTING
    engine.rootContext()->setContextProperty("manualTesting", true);
#else
    engine.rootContext()->setContextProperty("manualTesting", false);
#endif

    uiStateSerializer.setLinkManagerWrapper(core.getLinkManagerWrapperPtr());

    QObject::connect(&langController, &LanguageController::currentIndexChanged, &engine, [&engine, &app, &langController, &inputDeviceTracker]() {
        emit langController.aboutToRetranslate();
        engine.retranslate();
        setApplicationDisplayName(app);
        emit core.languageChanged();
        emit inputDeviceTracker.currentModeChanged();
        emit langController.retranslated();
    });

    QObject::connect(&theme, &Themes::interfaceChanged, &core, []() {
        core.setConsoleOutputEnabled(theme.consoleVisible());
    });
    core.setConsoleOutputEnabled(theme.consoleVisible());

    core.consoleInfo("Run...");
    core.setEngine(&engine);
    //qDebug() << "SQL drivers =" << QSqlDatabase::drivers(); // тут должен появиться QSQLITE
    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QPointer<QQuickWindow> mainWindow;
    QObject::connect(&engine,   &QQmlApplicationEngine::objectCreated,
                     &app,      [url](QObject *obj, const QUrl &objUrl) {
                                    if (!obj && url == objUrl)
                                        QCoreApplication::exit(-1);
                                }, Qt::QueuedConnection);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &uiStateSerializer, [url](QObject* obj, const QUrl& objUrl) {
        if (obj && url == objUrl) {
            uiStateSerializer.setQmlRootObject(obj);
        }
    }, Qt::QueuedConnection);

// file opening on startup
#ifndef Q_OS_ANDROID
    {
        const QStringList appArgs = app.arguments();
        QString startupFilePath;
        for (int i = 1; i < appArgs.size(); ++i) {
            if (!appArgs.at(i).startsWith(QLatin1String("--"))) {
                startupFilePath = appArgs.at(i);
                break;
            }
        }
        if (!startupFilePath.isEmpty()) {
            auto* startupConn = new QMetaObject::Connection;
            *startupConn = QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                                            &core, [startupFilePath, startupConn, url](QObject* obj, const QUrl& objUrl) {
                                                if (!obj || url != objUrl) return;
                                                QObject::disconnect(*startupConn);
                                                delete startupConn;
                                                core.deferStartupFileOpen(startupFilePath);
                                            }, Qt::QueuedConnection);
        }
    }
#endif

    engine.load(url);
    const auto rootObjects = engine.rootObjects();
    if (!rootObjects.isEmpty()) {
        QObject* rootObject = rootObjects.constFirst();
        mainWindow = qobject_cast<QQuickWindow*>(rootObject);
        if (mainWindow) {
            uiProbe.setWindow(mainWindow);
        }
        if (mainWindow && UiProbe::isEnabled()) {
            uiProbe.armFromEnvironment();
        }

        {
            ControlEndpoint endpoint;
            if (resolveControlEndpoint(app.arguments(), appUtils.instanceIndex(), endpoint)) {
                controlServer.registerObject(QStringLiteral("core"),                 &core);
                controlServer.registerObject(QStringLiteral("dataset"),              core.getDatasetPtr());
                controlServer.registerObject(QStringLiteral("theme"),                &theme);
                controlServer.registerObject(QStringLiteral("linkManagerWrapper"),   core.getLinkManagerWrapperPtr());
                controlServer.registerObject(QStringLiteral("deviceManagerWrapper"), core.getDeviceManagerWrapperPtr());
                controlServer.registerObject(QStringLiteral("deviceTopology"),       core.getDeviceTopologyModelPtr());
                controlServer.registerObject(QStringLiteral("videoStreams"),         &videoStreams);
                controlServer.registerObject(QStringLiteral("linkDiscovery"),        &linkDiscovery);
                controlServer.registerObject(QStringLiteral("logViewer"),            core.getConsolePtr());
                controlServer.registerObject(QStringLiteral("uiStateSerializer"),    &uiStateSerializer);
                controlServer.registerObject(QStringLiteral("echogramStateSerializer"), &echogramStateSerializer);
                controlServer.registerObject(QStringLiteral("notifications"),        &notifications);
                controlServer.registerObject(QStringLiteral("inputDeviceTracker"),   &inputDeviceTracker);
                controlServer.registerObject(QStringLiteral("systemBattery"),        &systemBattery);
                controlServer.registerObject(QStringLiteral("langController"),       &langController);
                controlServer.registerObject(QStringLiteral("appUtils"),             &appUtils);
                controlServer.registerObject(QStringLiteral("uiProbe"),              &uiProbe);
                controlServer.registerObject(QStringLiteral("root"),                 rootObject);
                if (auto* store = rootObject->findChild<QObject*>(QStringLiteral("workspaceStore"))) {
                    controlServer.registerObject(QStringLiteral("workspaceStore"), store);
                }
                static const char* const kSceneControllerRoots[] = {
                    "BoatTrackControlMenuController", "NavigationArrowControlMenuController",
                    "BottomTrackControlMenuController", "IsobathsViewControlMenuController",
                    "MosaicViewControlMenuController", "ImageViewControlMenuController",
                    "MapViewControlMenuController", "PointGroupControlMenuController",
                    "PolygonGroupControlMenuController", "MpcFilterControlMenuController",
                    "NpdFilterControlMenuController", "Scene3DControlMenuController",
                    "Scene3dToolBarController", "hotkeysController"};
                for (const char* name : kSceneControllerRoots) {
                    const QString key = QString::fromLatin1(name);
                    if (auto* obj = engine.rootContext()->contextProperty(key).value<QObject*>()) {
                        controlServer.registerObject(key, obj);
                    }
                }
                controlServer.setUiProbe(&uiProbe);
                controlServer.setArtifactRoot(endpoint.artifactRoot);
                controlServer.start(endpoint.address, endpoint.port, endpoint.token);
            }
        }
#if defined(Q_OS_WIN)
        if (auto* window = qobject_cast<QWindow*>(rootObject)) {
            applyWindowsSystemTitleBarTheme(window);
            applyWindowsFullscreenBorderWorkaround(window);
            bringWindowToFront(window);
        }
        QObject::connect(&core, &Core::bringWindowToFrontRequested, &app, [mainWindow]() { // runtime requests, next event-loop tick
            if (mainWindow) {
                bringWindowToFront(mainWindow);
            }
        }, Qt::QueuedConnection);
        // Same dark titlebar + fullscreen border workaround for the secondary window.
        if (auto* secondary = rootObject->findChild<QWindow*>(QStringLiteral("secondaryAppWindow"))) {
            applyWindowsSystemTitleBarTheme(secondary);
            applyWindowsFullscreenBorderWorkaround(secondary);
        }
        QObject::connect(&theme, &Themes::changed, &app, [mainWindow]() {
            if (!mainWindow) {
                return;
            }
            applyWindowsSystemTitleBarTheme(mainWindow);
            if (auto* secondary = mainWindow->findChild<QWindow*>(QStringLiteral("secondaryAppWindow"))) {
                applyWindowsSystemTitleBarTheme(secondary);
            }
        });
#endif
    }
    qInfo() << "App is created";
    const int retCode = app.exec();

    controlServer.stop();
    core.shutdownBackgroundWorkers();
    core.saveLLARefToSettings();
    core.removeLinkManagerConnections();
#ifdef SEPARATE_READING
    core.stopDeviceManagerThread();
#endif

    AppLog::instance().stop();

    return retCode;
}

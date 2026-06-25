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
#if defined(Q_OS_WIN)
#include <windows.h>
#endif
#include "qPlot2D.h"
#include "core.h"
#include "themes.h"
#include "ui_state_serializer.h"
#include "echogram_state_serializer.h"
#include "notifications.h"
#include "scene_object.h"
#include "bottom_track.h"
#include "input_device_tracker.h"
#include "language_controller.h"
#include "app_utils.h"


Core core;
AppUtils appUtils;
Themes theme;
UIStateSerializer uiStateSerializer;
EchogramStateSerializer echogramStateSerializer;
Notifications notifications;
QTranslator translator;
QVector<QString> availableLanguages{"en", "ru", "pl"};


void loadLanguage(QGuiApplication &app)
{
    QSettings settings;
    QString currentLanguage;

    int savedLanguageIndex = settings.value("appLanguage", -1).toInt();

    if (savedLanguageIndex == -1) {
        currentLanguage = QLocale::system().name().split('_').first();
        if (auto indx = availableLanguages.indexOf(currentLanguage); indx == -1) {
            currentLanguage = availableLanguages.front();
        }
        else {
            settings.setValue("appLanguage", indx);
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

void applyWindowsSystemTitleBarTheme(QWindow* window)
{
    if (!window) {
        return;
    }

    const HWND handle = reinterpret_cast<HWND>(window->winId());
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

    const BOOL useDarkCaption = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark ? TRUE : FALSE;
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

    FreeLibrary(dwmApi);
}

void applyWindowsFullscreenBorderWorkaround(QWindow* window)
{
    if (!window) {
        return;
    }

    auto applyBorder = [window]() {
        HWND handle = reinterpret_cast<HWND>(window->winId());
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
    if (!window) {
        return;
    }

    window->raise();
    window->requestActivate();

    const HWND handle = reinterpret_cast<HWND>(window->winId());
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
    SystemParametersInfoW(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, reinterpret_cast<PVOID>(static_cast<UINT_PTR>(0)), SPIF_SENDCHANGE);

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
                          reinterpret_cast<PVOID>(static_cast<UINT_PTR>(savedLockTimeout)), SPIF_SENDCHANGE);

    // flash taskbar button
    FLASHWINFO flash = {};
    flash.cbSize = sizeof(flash);
    flash.hwnd = handle;
    flash.dwFlags = FLASHW_TRAY | FLASHW_TIMERNOFG;
    FlashWindowEx(&flash);
}
#endif


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

#if defined(Q_OS_WIN)
    //QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);
    QLoggingCategory::setFilterRules(QStringLiteral("qt.network.info.netlistmanager.warning=false"));
#endif

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

    // One-shot migration: the "Chart" group (multi-plot + synchronisation) was
    // removed from the new settings UI. Force any persisted multi-plot OR
    // sync-on state back to single-plot, no-sync. OR (not AND) — otherwise
    // users with numPlots=2,sync=false stay stuck in a layout the new UI
    // can no longer toggle off.
    {
        QSettings s;
        const bool needMigration =
            s.value("numPlotsSpinBox", 1).toInt() >= 2 ||
            s.value("plotSyncCheckBox", false).toBool();
        if (needMigration) {
            s.setValue("numPlotsSpinBox", 1);
            s.setValue("plotSyncCheckBox", false);
        }
    }

    LanguageController langController;
    InputDeviceTracker inputDeviceTracker;
    core.initAfterApp();

    //qDebug() << "Lib paths:" << QCoreApplication::libraryPaths();
    //qDebug() << "SQL drivers:" << QSqlDatabase::drivers();

    QCoreApplication::addLibraryPath(QStringLiteral("assets:/qt/plugins"));
    QCoreApplication::addLibraryPath(QStringLiteral(":/android_rcc_bundle/plugins"));
    //qputenv("QT_DEBUG_PLUGINS", "1");
    //qDebug() << "libraryPaths =" << QCoreApplication::libraryPaths();
    loadLanguage(app);
    langController.setStartupTranslator(&translator);
    core.initStreamList();

    QQuickStyle::setStyle("Basic");

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
    engine.rootContext()->setContextProperty("logViewer", core.getConsolePtr());
    engine.rootContext()->setContextProperty("uiStateSerializer", &uiStateSerializer);
    engine.rootContext()->setContextProperty("echogramStateSerializer", &echogramStateSerializer);
    engine.rootContext()->setContextProperty("notifications", &notifications);
    engine.rootContext()->setContextProperty("inputDeviceTracker", &inputDeviceTracker);
    engine.rootContext()->setContextProperty("langController", &langController);
    engine.rootContext()->setContextProperty("appUtils", &appUtils);

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
        if (appArgs.size() > 1) {
            const QString startupFilePath = appArgs.at(1);
            auto* startupConn = new QMetaObject::Connection;
            *startupConn = QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                                            &core, [startupFilePath, startupConn, url](QObject* obj, const QUrl& objUrl) {
                                                if (!obj || url != objUrl) return;
                                                QObject::disconnect(*startupConn);
                                                delete startupConn;
                                                core.openLogFile(startupFilePath, false, true);
                                            }, Qt::QueuedConnection);
        }
    }
#endif

    engine.load(url);
    const auto rootObjects = engine.rootObjects();
    if (!rootObjects.isEmpty()) {
        QObject* rootObject = rootObjects.constFirst();
        mainWindow = qobject_cast<QQuickWindow*>(rootObject);
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
#endif
    }
    qInfo() << "App is created";
    const int retCode = app.exec();

    core.shutdownBackgroundWorkers();
    core.saveLLARefToSettings();
    core.removeLinkManagerConnections();
#ifdef SEPARATE_READING
    core.stopDeviceManagerThread();
#endif

    return retCode;
}

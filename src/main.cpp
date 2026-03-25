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
#if defined(Q_OS_WIN)
#include <windows.h>
#endif
#include "qPlot2D.h"
#include "core.h"
#include "themes.h"
#include "ui_state_serializer.h"
#include "scene_object.h"
#include "bottom_track.h"


Core core;
Themes theme;
UIStateSerializer uiStateSerializer;
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
#endif


int main(int argc, char *argv[])
{
#ifdef Q_OS_ANDROID
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0");  // TODO: use qt scaling!
    qputenv("QT_SCALE_FACTOR", "0.5");            //
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
    core.initAfterApp();

    //qDebug() << "Lib paths:" << QCoreApplication::libraryPaths();
    //qDebug() << "SQL drivers:" << QSqlDatabase::drivers();

    QCoreApplication::addLibraryPath(QStringLiteral("assets:/qt/plugins"));
    QCoreApplication::addLibraryPath(QStringLiteral(":/android_rcc_bundle/plugins"));
    //qputenv("QT_DEBUG_PLUGINS", "1");
    //qDebug() << "libraryPaths =" << QCoreApplication::libraryPaths();
    loadLanguage(app);
    core.initStreamList();

    QQuickStyle::setStyle("Basic");

    setApplicationDisplayName(app);
    QQmlApplicationEngine engine;
    engine.addImportPath("qrc:/");

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
    uiStateSerializer.setLinkManagerWrapper(core.getLinkManagerWrapperPtr());

    QObject::connect(&theme, &Themes::interfaceChanged, &core, []() {
        core.setConsoleOutputEnabled(theme.consoleVisible());
    });
    core.setConsoleOutputEnabled(theme.consoleVisible());

    core.consoleInfo("Run...");
    core.setEngine(&engine);
    //qDebug() << "SQL drivers =" << QSqlDatabase::drivers(); // тут должен появиться QSQLITE
    const QUrl url(QStringLiteral("qrc:/main.qml"));
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
    if (argc > 1) {
        QObject::connect(&engine,   &QQmlApplicationEngine::objectCreated,
                         &core,     [&argv]() {
                                        core.openLogFile(argv[1], false, true);
                                    }, Qt::QueuedConnection);
    }
#endif

    QObject::connect(&app,  &QGuiApplication::aboutToQuit,
                     &core, [&]() {
                                core.shutdownDataProcessor();
                                core.saveLLARefToSettings();
                                core.removeLinkManagerConnections();
                                core.stopLinkManagerTimer();
#ifdef SEPARATE_READING
                                void removeDeviceManagerConnections();
                                core.stopDeviceManagerThread();
#endif
                            });

    engine.load(url);
    const auto rootObjects = engine.rootObjects();
    if (!rootObjects.isEmpty()) {
        QObject* rootObject = rootObjects.constFirst();
        mainWindow = qobject_cast<QQuickWindow*>(rootObject);
#if defined(Q_OS_WIN)
        if (auto* window = qobject_cast<QWindow*>(rootObject)) {
            applyWindowsSystemTitleBarTheme(window);
            applyWindowsFullscreenBorderWorkaround(window);
        }
#endif
    }
    qInfo() << "App is created";
    return app.exec();
}

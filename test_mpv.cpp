#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <clocale>

#include "mpv.h"
#include "stremioprocess.h"
#include "screensaver.h"
#include "qclipboardproxy.h"
#include "systemtray.h"

int main(int argc, char **argv)
{
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--autoplay-policy=no-user-gesture-required");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QApplication app(argc, argv);
    app.setApplicationName("Stremio");
    app.setApplicationVersion("4.4.183");
    app.setOrganizationName("Smart Code ltd");

    std::setlocale(LC_NUMERIC, "C");

    qmlRegisterType<Process>("com.stremio.process", 1, 0, "Process");
    qmlRegisterType<ScreenSaver>("com.stremio.screensaver", 1, 0, "ScreenSaver");
    qmlRegisterType<MpvObject>("com.stremio.libmpv", 1, 0, "MpvObject");
    qmlRegisterType<ClipboardProxy>("com.stremio.clipboard", 1, 0, "Clipboard");

    QQmlApplicationEngine engine;

    SystemTray *systemTray = new SystemTray();
    QQmlContext *ctx = engine.rootContext();
    ctx->setContextProperty("applicationDirPath", QApplication::applicationDirPath());
    ctx->setContextProperty("appTitle", "Stremio - Freedom to Stream");
    ctx->setContextProperty("systemTray", systemTray);
    ctx->setContextProperty("debug", false);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}

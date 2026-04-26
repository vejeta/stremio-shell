#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <clocale>

#include "mpv.h"

int main(int argc, char **argv)
{
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QApplication app(argc, argv);
    app.setApplicationName("MpvTest");

    std::setlocale(LC_NUMERIC, "C");

    qmlRegisterType<MpvObject>("com.stremio.libmpv", 1, 0, "MpvObject");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/test_mpv.qml")));

    return app.exec();
}

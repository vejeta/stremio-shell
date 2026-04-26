#include "mpv.h"

#include <clocale>
#include <QJsonObject>
#include <QJsonValue>
#include <QQuickWindow>

MpvObject::MpvObject(QQuickItem *parent)
    : MpvAbstractItem(parent)
{
    std::setlocale(LC_NUMERIC, "C");

    // The player is hidden by default. It is shown only when a video stream is available
    this->setVisible(false);

    setupConnections();

    // Wait for MpvQt renderer to be ready before configuring mpv
    connect(this, &MpvAbstractItem::ready, this, [this]() {
        auto *ctrl = mpvController();

        ctrl->setProperty("vo", "libmpv");
        ctrl->setProperty("gpu-hwdec-interop", "auto");
        ctrl->setProperty("terminal", "yes");
        ctrl->setProperty("msg-level", "all=v");

        ctrl->setProperty("cache-default", 15000);
        ctrl->setProperty("cache-backbuffer", 15000);
        ctrl->setProperty("cache-secs", 10);

        ctrl->setProperty("audio-client-name", QCoreApplication::applicationName());
        ctrl->setProperty("title", QCoreApplication::applicationName());
        ctrl->setProperty("audio-fallback-to-null", "yes");

        observeProperty("vid");
    });
}

void MpvObject::setupConnections()
{
    auto *ctrl = mpvController();

    connect(ctrl, &MpvController::propertyChanged,
            this, &MpvObject::onPropertyChanged);
    connect(ctrl, &MpvController::fileLoaded,
            this, &MpvObject::onFileLoaded);
    connect(ctrl, &MpvController::endFile,
            this, &MpvObject::onEndFile);
}

void MpvObject::onPropertyChanged(const QString &property, const QVariant &value)
{
    QJsonObject eventJson;
    eventJson["name"] = property;
    eventJson["data"] = QJsonValue::fromVariant(value);

    // Show the player only if there is a video stream
    if (property == "vid" && value.canConvert<qlonglong>())
        this->setVisible(true);

    Q_EMIT mpvEvent("mpv-prop-change", eventJson);
}

void MpvObject::onFileLoaded()
{
    // File loaded successfully
}

void MpvObject::onEndFile(const QString &reason)
{
    this->setVisible(false);

    QJsonObject eventJson;
    eventJson["reason"] = reason;
    Q_EMIT mpvEvent("mpv-event-ended", eventJson);
}

void MpvObject::command(const QVariant &params)
{
    if (params.canConvert<QStringList>()) {
        MpvAbstractItem::command(params.toStringList());
    } else if (params.canConvert<QVariantList>()) {
        QStringList args;
        for (const auto &v : params.toList())
            args << v.toString();
        MpvAbstractItem::command(args);
    }
}

void MpvObject::setProperty(const QString &name, const QVariant &value)
{
    MpvAbstractItem::setProperty(name, value);
}

QVariant MpvObject::getProperty(const QString &name)
{
    return MpvAbstractItem::getProperty(name);
}

void MpvObject::observeProperty(const QString &name)
{
    MpvAbstractItem::observeProperty(name, MPV_FORMAT_NODE);
}

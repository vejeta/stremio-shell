#include "mpv.h"

#include <stdexcept>
#include <clocale>

#include <QObject>
#include <QJsonObject>

#include <QtGlobal>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QGuiApplication>

#include <QtQuick/QQuickWindow>

#if defined(Q_OS_WIN32)
#include <windows.h>
#include <dwmapi.h>
#pragma comment (lib, "dwmapi.lib")
#endif

namespace
{
void on_mpv_redraw(void *ctx)
{
    MpvObject::on_update(ctx);
}

static void *get_proc_address_mpv(void *ctx, const char *name)
{
    Q_UNUSED(ctx)

    QOpenGLContext *glctx = QOpenGLContext::currentContext();
    if (!glctx)
        return nullptr;

    return reinterpret_cast<void *>(glctx->getProcAddress(QByteArray(name)));
}

} // namespace


MpvObject::MpvObject(QQuickItem * parent)
    : QQuickItem(parent), mpv{mpv_create()}, mpv_gl(nullptr)
{
#ifdef Q_OS_WIN32
    DwmEnableMMCSS(TRUE);
#endif

    if (!mpv)
        throw std::runtime_error("could not create mpv context");

    std::setlocale(LC_NUMERIC, "C");

    connect(this, &MpvObject::onUpdate, this, &MpvObject::doUpdate,
            Qt::QueuedConnection);

    initialize_mpv();

    this->setVisible(false);
    this->observeProperty("vid");

    connect(this, &QQuickItem::windowChanged, this, &MpvObject::handleWindowChanged);
}

MpvObject::~MpvObject()
{
    if (mpv_gl)
    {
        mpv_render_context_free(mpv_gl);
    }

    mpv_terminate_destroy(mpv);
}

// Following the Qt6 official "OpenGL Under QML" example pattern:
// https://doc.qt.io/qt-6/qtquick-scenegraph-openglunderqml-example.html
void MpvObject::handleWindowChanged(QQuickWindow *win)
{
    if (!win)
        return;

    connect(win, &QQuickWindow::beforeSynchronizing, this, &MpvObject::sync,
            Qt::DirectConnection);
    connect(win, &QQuickWindow::sceneGraphInvalidated, this, &MpvObject::cleanup,
            Qt::DirectConnection);
    connect(win, &QQuickWindow::beforeRenderPassRecording, this, &MpvObject::renderMpv,
            Qt::DirectConnection);
}

void MpvObject::sync()
{
    // Create render context on first sync (GL context is current on render thread)
    if (!mpv_gl)
    {
        mpv_opengl_init_params gl_init_params{get_proc_address_mpv, nullptr, nullptr};

        mpv_render_param display{MPV_RENDER_PARAM_INVALID, nullptr};
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
        if (QGuiApplication::platformName() == QStringLiteral("xcb")) {
            display.type = MPV_RENDER_PARAM_X11_DISPLAY;
            display.data = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->display();
        }
        if (QGuiApplication::platformName() == QStringLiteral("wayland")) {
            display.type = MPV_RENDER_PARAM_WL_DISPLAY;
            display.data = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>()->display();
        }
#endif

        mpv_render_param params[]{
            {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
            {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
            display,
            {MPV_RENDER_PARAM_INVALID, nullptr}};

        if (mpv_render_context_create(&mpv_gl, mpv, params) < 0)
            throw std::runtime_error("failed to initialize mpv GL context");
        mpv_render_context_set_update_callback(mpv_gl, on_mpv_redraw, this);
    }
}

void MpvObject::renderMpv()
{
    if (!mpv_gl)
        return;

    QQuickWindow *win = window();
    QSize size = win->size() * win->devicePixelRatio();

    win->beginExternalCommands();

    // Render to whatever FBO Qt6 RHI is currently targeting
    GLint currentFbo = 0;
    QOpenGLContext::currentContext()->functions()->glGetIntegerv(
        GL_FRAMEBUFFER_BINDING, &currentFbo);

    mpv_opengl_fbo mpfbo{currentFbo, size.width(), size.height(), 0};
    int flip_y{1};

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
        {MPV_RENDER_PARAM_INVALID, nullptr}};

    mpv_render_context_render(mpv_gl, params);

    win->endExternalCommands();
}

void MpvObject::cleanup()
{
    if (mpv_gl)
    {
        mpv_render_context_free(mpv_gl);
        mpv_gl = nullptr;
    }
}

void MpvObject::initialize_mpv() {
    mpv_set_option_string(mpv, "terminal", "yes");
    mpv_set_option_string(mpv, "msg-level", "all=v");

    if (mpv_initialize(mpv) < 0)
        throw std::runtime_error("could not initialize mpv context");

    mpv::qt::set_property(mpv, "vo", "libmpv");
    mpv::qt::set_property(mpv, "gpu-hwdec-interop", "auto");

    mpv::qt::set_property(mpv, "cache-default", 15000);
    mpv::qt::set_property(mpv, "cache-backbuffer", 15000);
    mpv::qt::set_property(mpv, "cache-secs", 10);

    mpv::qt::set_property(mpv, "audio-client-name", QCoreApplication::applicationName());
    mpv::qt::set_property(mpv, "title", QCoreApplication::applicationName());
    mpv::qt::set_property(mpv, "audio-fallback-to-null", "yes");

    mpv_set_wakeup_callback(mpv, wakeup, this);

    foreach (const QString &name, observed_properties) {
        mpv_observe_property(mpv, 0, name.toStdString().c_str(), MPV_FORMAT_NODE);
    }
}

void MpvObject::on_update(void *ctx)
{
    MpvObject *self = (MpvObject *)ctx;
    emit self->onUpdate();
}

void MpvObject::doUpdate()
{
    if (window())
        window()->update();
}

void MpvObject::command(const QVariant& params)
{
    mpv::qt::command(mpv, params);
}

void MpvObject::setProperty(const QString& name, const QVariant& value)
{
    mpv::qt::set_property(mpv, name, value);
}

void MpvObject::observeProperty(const QString& name)
{
    observed_properties.insert(name);
    mpv_observe_property(mpv, 0, name.toStdString().c_str(), MPV_FORMAT_NODE);
}

void MpvObject::wakeup(void *ctx)
{
    QMetaObject::invokeMethod((MpvObject*)ctx, "on_mpv_events", Qt::QueuedConnection);
}

void MpvObject::on_mpv_events()
{
    while (mpv) {
        mpv_event *event = mpv_wait_event(mpv, 0);
        if (event->event_id == MPV_EVENT_NONE) {
            break;
        }
        handle_mpv_event(event);
    }
}

void MpvObject::handle_mpv_event(mpv_event *event) {
    QJsonObject eventJson;

    eventJson["id"] = qint64(event->reply_userdata);

    if (event->error < 0)
        eventJson["error"] = QString(mpv_error_string(event->error));

    switch (event->event_id) {
        case MPV_EVENT_PROPERTY_CHANGE: {
            mpv_event_property *prop = (mpv_event_property *) event->data;
            eventJson["name"] = QString(prop->name);

            switch (prop->format) {
            case MPV_FORMAT_NODE:
                if(((mpv_node *)prop->data)->format == MPV_FORMAT_INT64 && eventJson["name"] == "vid")
                    this->setVisible(true);
                eventJson["data"] = QJsonValue::fromVariant(mpv::qt::node_to_variant((mpv_node *) prop->data));
                break;
            case MPV_FORMAT_DOUBLE:
                eventJson["data"] = *(double *)prop->data;
                break;
            case MPV_FORMAT_FLAG:
                eventJson["data"] = *(int *)prop->data;
                break;
            case MPV_FORMAT_STRING:
                eventJson["data"] = QString(*(char **)prop->data);
                break;
            default:
                break;
            }

            Q_EMIT mpvEvent("mpv-prop-change", eventJson);
            break;
        }
        case MPV_EVENT_END_FILE: {
            this->setVisible(false);
            mpv_event_end_file *endFile = (mpv_event_end_file *)event->data;
            switch (endFile->reason) {
                case MPV_END_FILE_REASON_ERROR:
                    eventJson["reason"] = "error";
                    eventJson["error"] = mpv_error_string(endFile->error);
                    break;
                case MPV_END_FILE_REASON_QUIT:
                    eventJson["reason"] = "quit";
                    break;
                default:
                    eventJson["reason"] = "other";
                    break;
            }
            Q_EMIT mpvEvent("mpv-event-ended", eventJson);
            break;
        }
        case MPV_EVENT_SHUTDOWN: {
            if (mpv_gl)
            {
                mpv_render_context_free(mpv_gl);
                mpv_gl = nullptr;
            }
            mpv_terminate_destroy(mpv);
            mpv = mpv_create();
            initialize_mpv();
            break;
        }
        default: {
            break;
        }
    }
}

QVariant MpvObject::getProperty(const QString& name) {
    return mpv::qt::get_property(mpv, name);
}

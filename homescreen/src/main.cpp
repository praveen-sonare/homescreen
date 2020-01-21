/*
 * Copyright (C) 2016, 2017 Mentor Graphics Development (Deutschland) GmbH
 * Copyright (c) 2017, 2018 TOYOTA MOTOR CORPORATION
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <QGuiApplication>
#include <QCommandLineParser>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlComponent>
#include <QtQml/qqml.h>
#include <QQuickWindow>
#include <QScreen>
#include <QUrlQuery>
#include <QTimer>
#include <qpa/qplatformnativeinterface.h>

#include <cstdlib>
#include <cstring>
#include <memory>
#include <wayland-client.h>

#include <weather.h>
#include <bluetooth.h>
#include "applicationlauncher.h"
#include "statusbarmodel.h"
#include "mastervolume.h"
#include "shell.h"
#include "hmi-debug.h"

#include "wayland-agl-shell-client-protocol.h"

#define CONNECT_STR	"unix:/run/platform/apis/ws/afm-main"

static void global_add(void *data, struct wl_registry *reg, uint32_t name,
                       const char *interface, uint32_t)
{
    struct agl_shell **shell = static_cast<struct agl_shell **>(data);
    if (strcmp(interface, agl_shell_interface.name) == 0) {
        *shell = static_cast<struct agl_shell *>(wl_registry_bind(reg, name, &agl_shell_interface, 1));
    }
}

static void global_remove(void *, struct wl_registry *, uint32_t)
{
    // Don't care
}

static const struct wl_registry_listener registry_listener = {
    global_add,
    global_remove,
};

static struct wl_surface *create_component(QPlatformNativeInterface *native,
                                           QQmlComponent *comp, QScreen *screen)
{
    QObject *obj = comp->create();
    obj->setParent(screen);

    QWindow *win = qobject_cast<QWindow *>(obj);
    return static_cast<struct wl_surface *>(native->nativeResourceForWindow("surface", win));
}

int main(int argc, char *argv[])
{
    setenv("QT_QPA_PLATFORM", "wayland", 1);
    QGuiApplication a(argc, argv);
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    struct wl_display *wl;
    struct wl_registry *registry;
    struct agl_shell *agl_shell = nullptr;

    wl = static_cast<struct wl_display *>(native->nativeResourceForIntegration("display"));
    registry = wl_display_get_registry(wl);

    wl_registry_add_listener(registry, &registry_listener, &agl_shell);
    // Roundtrip to get all globals advertised by the compositor
    wl_display_roundtrip(wl);
    wl_registry_destroy(registry);

    if (!agl_shell) {
        qFatal("Compositor does not support AGL shell protocol");
        return 1;
    }
    std::shared_ptr<struct agl_shell> shell{agl_shell, agl_shell_destroy};

    QCoreApplication::setOrganizationDomain("LinuxFoundation");
    QCoreApplication::setOrganizationName("AutomotiveGradeLinux");
    QCoreApplication::setApplicationName("HomeScreen");
    QCoreApplication::setApplicationVersion("0.7.0");

    QCommandLineParser parser;
    parser.addPositionalArgument("port", a.translate("main", "port for binding"));
    parser.addPositionalArgument("secret", a.translate("main", "secret for binding"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(a);
    QStringList positionalArguments = parser.positionalArguments();

    int port = 1700;
    QString token = "wm";
    QString graphic_role = "homescreen"; // defined in layers.json in Window Manager

    if (positionalArguments.length() == 2) {
        port = positionalArguments.takeFirst().toInt();
        token = positionalArguments.takeFirst();
    }

    HMI_DEBUG("HomeScreen","port = %d, token = %s", port, token.toStdString().c_str());

    // import C++ class to QML
    // qmlRegisterType<ApplicationLauncher>("HomeScreen", 1, 0, "ApplicationLauncher");
    qmlRegisterType<StatusBarModel>("HomeScreen", 1, 0, "StatusBarModel");
    qmlRegisterType<MasterVolume>("MasterVolume", 1, 0, "MasterVolume");

    ApplicationLauncher *launcher = new ApplicationLauncher(CONNECT_STR, &a);

    QUrl bindingAddress;
    bindingAddress.setScheme(QStringLiteral("ws"));
    bindingAddress.setHost(QStringLiteral("localhost"));
    bindingAddress.setPort(port);
    bindingAddress.setPath(QStringLiteral("/api"));

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("token"), token);
    bindingAddress.setQuery(query);

    QQmlEngine engine;
    QQmlContext *context = engine.rootContext();
    context->setContextProperty("bindingAddress", bindingAddress);
    context->setContextProperty("launcher", launcher);
    context->setContextProperty("weather", new Weather(bindingAddress));
    context->setContextProperty("bluetooth", new Bluetooth(bindingAddress, engine.rootContext()));
    context->setContextProperty("shell", new Shell(shell, &a));

    QQmlComponent bg_comp(&engine, QUrl("qrc:/background.qml"));
    qInfo() << bg_comp.errors();

    QQmlComponent top_comp(&engine, QUrl("qrc:/toppanel.qml"));
    qInfo() << top_comp.errors();

    QQmlComponent bot_comp(&engine, QUrl("qrc:/bottompanel.qml"));
    qInfo() << bot_comp.errors();

    for (QScreen *screen : qApp->screens()) {
        struct wl_output *output;

        output = static_cast<struct wl_output *>(native->nativeResourceForScreen("output", screen));

        struct wl_surface *bg = create_component(native, &bg_comp, screen);
        struct wl_surface *top = create_component(native, &top_comp, screen);
        struct wl_surface *bot = create_component(native, &bot_comp, screen);

	wl_display_dispatch(wl);

        agl_shell_set_panel(agl_shell, top, output, AGL_SHELL_EDGE_TOP);
        agl_shell_set_panel(agl_shell, bot, output, AGL_SHELL_EDGE_BOTTOM);
        agl_shell_set_background(agl_shell, bg, output);
    }

    // Delay the ready signal until after Qt has done all of its own setup in a.exec()
    QTimer::singleShot(0, [shell](){
        agl_shell_ready(shell.get());
    });

    return a.exec();
}

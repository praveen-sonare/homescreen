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
#include <QtCore/QUrlQuery>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlComponent>
#include <QtQml/qqml.h>
#include <QQuickWindow>
#include <QTimer>

#include <weather.h>
#include <bluetooth.h>
#include "applicationlauncher.h"
#include "statusbarmodel.h"
#include "mastervolume.h"
#include "homescreenhandler.h"
#include "hmi-debug.h"
#include "chromecontroller.h"

#include <qpa/qplatformnativeinterface.h>
#include <wayland-client.h>

#include "wayland-agl-shell-client-protocol.h"
#include "shell.h"

static void
global_add(void *data, struct wl_registry *reg, uint32_t name,
	   const char *interface, uint32_t)
{
	struct agl_shell **shell = static_cast<struct agl_shell **>(data);

	if (strcmp(interface, agl_shell_interface.name) == 0) {
		*shell = static_cast<struct agl_shell *>(
			wl_registry_bind(reg, name, &agl_shell_interface, 1)
		);
	}
}

static void
global_remove(void *data, struct wl_registry *reg, uint32_t id)
{
	/* Don't care */
	(void) data;
	(void) reg;
	(void) id;
}

static const struct wl_registry_listener registry_listener = {
	global_add,
	global_remove,
};

static struct wl_surface *
getWlSurface(QPlatformNativeInterface *native, QWindow *window)
{
	void *surf = native->nativeResourceForWindow("surface", window);
	return static_cast<struct ::wl_surface *>(surf);
}

static struct wl_output *
getWlOutput(QPlatformNativeInterface *native, QScreen *screen)
{
	void *output = native->nativeResourceForScreen("output", screen);
	return static_cast<struct ::wl_output*>(output);
}


static struct agl_shell *
register_agl_shell(QPlatformNativeInterface *native)
{
	struct wl_display *wl;
	struct wl_registry *registry;
	struct agl_shell *shell = nullptr;

	wl = static_cast<struct wl_display *>(
			native->nativeResourceForIntegration("display")
	);
	registry = wl_display_get_registry(wl);

	wl_registry_add_listener(registry, &registry_listener, &shell);

	/* Roundtrip to get all globals advertised by the compositor */
	wl_display_roundtrip(wl);
	wl_registry_destroy(registry);

	return shell;
}

static struct wl_surface *
create_component(QPlatformNativeInterface *native, QQmlComponent *comp,
		 QScreen *screen, QObject **qobj)
{
	QObject *obj = comp->create();
	obj->setParent(screen);

	QWindow *win = qobject_cast<QWindow *>(obj);
	*qobj = obj;

	return getWlSurface(native, win);
}

static void
load_agl_shell_app(QPlatformNativeInterface *native,
		   QQmlApplicationEngine *engine,
		   struct agl_shell *agl_shell, QUrl &bindingAddress)
{
	struct wl_surface *bg, *top, *bottom;
	struct wl_output *output;

	QObject *qobj_bg, *qobj_top, *qobj_bottom;

	QQmlComponent bg_comp(engine, QUrl("qrc:/background.qml"));
	qInfo() << bg_comp.errors();

	QQmlComponent top_comp(engine, QUrl("qrc:/toppanel.qml"));
	qInfo() << top_comp.errors();

	QQmlComponent bot_comp(engine, QUrl("qrc:/bottompanel.qml"));
	qInfo() << bot_comp.errors();

	QScreen *screen = qApp->screens().first();
	if (!screen)
		return;

	output = getWlOutput(native, screen);

	bg = create_component(native, &bg_comp, screen, &qobj_bg);
	top = create_component(native, &top_comp, screen, &qobj_top);
	bottom = create_component(native, &bot_comp, screen, &qobj_bottom);

	/* engine.rootObjects() works only if we had a load() */
	StatusBarModel *statusBar = qobj_top->findChild<StatusBarModel *>("statusBar");
	if (statusBar) {
		qDebug() << "got statusBar objectname, doing init()";
		statusBar->init(bindingAddress, engine->rootContext());
	}

	agl_shell_set_panel(agl_shell, top, output, AGL_SHELL_EDGE_TOP);
	agl_shell_set_panel(agl_shell, bottom, output, AGL_SHELL_EDGE_BOTTOM);

	agl_shell_set_background(agl_shell, bg, output);

	/* Delay the ready signal until after Qt has done all of its own setup
	 * in a.exec() */
	QTimer::singleShot(500, [agl_shell](){
		agl_shell_ready(agl_shell);
	});
}


int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    struct agl_shell *agl_shell = nullptr;

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

    agl_shell = register_agl_shell(native);
    if (!agl_shell) {
	    fprintf(stderr, "agl_shell extension is not advertised. "
			    "Are you sure that agl-compositor is running?\n");
	    exit(EXIT_FAILURE);
    }

    std::shared_ptr<struct agl_shell> shell{agl_shell, agl_shell_destroy};
    Shell *aglShell = new Shell(shell, &a);

    // import C++ class to QML
    // qmlRegisterType<ApplicationLauncher>("HomeScreen", 1, 0, "ApplicationLauncher");
    qmlRegisterType<StatusBarModel>("HomeScreen", 1, 0, "StatusBarModel");
    qmlRegisterType<MasterVolume>("MasterVolume", 1, 0, "MasterVolume");
    qmlRegisterUncreatableType<ChromeController>("SpeechChrome", 1, 0, "SpeechChromeController",
                                                 QLatin1String("SpeechChromeController is uncreatable."));

    ApplicationLauncher *launcher = new ApplicationLauncher();

    HomescreenHandler* homescreenHandler = new HomescreenHandler(aglShell);
    homescreenHandler->init(port, token.toStdString().c_str());

    QUrl bindingAddress;
    bindingAddress.setScheme(QStringLiteral("ws"));
    bindingAddress.setHost(QStringLiteral("localhost"));
    bindingAddress.setPort(port);
    bindingAddress.setPath(QStringLiteral("/api"));

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("token"), token);
    bindingAddress.setQuery(query);

    QQmlApplicationEngine engine;
    QQmlContext *context = engine.rootContext();
    context->setContextProperty("bindingAddress", bindingAddress);

    context->setContextProperty("homescreenHandler", homescreenHandler);
    context->setContextProperty("launcher", launcher);
    context->setContextProperty("weather", new Weather(bindingAddress));
    context->setContextProperty("bluetooth", new Bluetooth(bindingAddress, context));
    context->setContextProperty("speechChromeController", new ChromeController(bindingAddress, &engine));
    // we add it here even if we don't use it
    context->setContextProperty("shell", aglShell);

    /* instead of loading main.qml we load one-by-one each of the QMLs,
     * divided now between several surfaces: panels, background.
     */
    load_agl_shell_app(native, &engine, agl_shell, bindingAddress);

    return a.exec();
}

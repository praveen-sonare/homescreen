// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright (C) 2016, 2017 Mentor Graphics Development (Deutschland) GmbH
 * Copyright (c) 2017, 2018 TOYOTA MOTOR CORPORATION
 * Copyright (c) 2022 Konsulko Group
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

#include <qpa/qplatformnativeinterface.h>
#include <wayland-client.h>

#include "wayland-agl-shell-client-protocol.h"
#include "wayland-agl-shell-desktop-client-protocol.h"
#include "shell.h"

struct shell_data {
	struct agl_shell *shell;
	struct agl_shell_desktop *shell_desktop;
};

static void
agl_shell_desktop_application(void *data,
			      struct agl_shell_desktop *agl_shell_desktop,
			      const char *app_id)
{
	HomescreenHandler *homescreenHandler = static_cast<HomescreenHandler *>(data);

	if (homescreenHandler)
		homescreenHandler->addAppToStack(app_id);
}

static void
agl_shell_desktop_state_app(void *data,
			    struct agl_shell_desktop *agl_shell_desktop,
			    const char *app_id,
			    const char *app_data,
			    uint32_t state,
			    uint32_t role)
{
	HomescreenHandler *homescreenHandler = static_cast<HomescreenHandler *>(data);

	if (homescreenHandler && state == AGL_SHELL_DESKTOP_APP_STATE_DESTROYED)
		homescreenHandler->deactivateApp(app_id);
}

static const struct agl_shell_desktop_listener shell_desktop_listener = {
   agl_shell_desktop_application,
   agl_shell_desktop_state_app
};

static void
global_add(void *data, struct wl_registry *reg, uint32_t name,
	   const char *interface, uint32_t)
{
	struct shell_data *shell_data = static_cast<struct shell_data *>(data);

	if (!shell_data)
		return;

	if (strcmp(interface, agl_shell_interface.name) == 0) {
		shell_data->shell = static_cast<struct agl_shell *>(
			wl_registry_bind(reg, name, &agl_shell_interface, 1)
		);
	}

	if (strcmp(interface, agl_shell_desktop_interface.name) == 0) {
		shell_data->shell_desktop = static_cast<struct agl_shell_desktop *>(
			wl_registry_bind(reg, name, &agl_shell_desktop_interface, 1)
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


static void
register_agl_shell(QPlatformNativeInterface *native, struct shell_data *shell_data)
{
	struct wl_display *wl;
	struct wl_registry *registry;

	wl = static_cast<struct wl_display *>(
			native->nativeResourceForIntegration("display")
	);
	registry = wl_display_get_registry(wl);

	wl_registry_add_listener(registry, &registry_listener, shell_data);

	/* Roundtrip to get all globals advertised by the compositor */
	wl_display_roundtrip(wl);
	wl_registry_destroy(registry);
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

static QScreen *
find_screen(const char *screen_name)
{
	QList<QScreen *> screens = qApp->screens();
	QScreen *found = nullptr;
	QString qstr_name = QString::fromUtf8(screen_name, -1);

	for (QScreen *xscreen : screens) {
		if (qstr_name == xscreen->name()) {
			found = xscreen;
			break;
		}
	}

	return found;
}

static void
load_agl_shell_app(QPlatformNativeInterface *native,
		   QQmlApplicationEngine *engine,
		   struct agl_shell *agl_shell,
		   const char *screen_name,
		    bool is_demo)
{
	struct wl_surface *bg, *top, *bottom;
	struct wl_output *output;
	QObject *qobj_bg, *qobj_top, *qobj_bottom;
	QScreen *screen = nullptr;

	if (is_demo) {
		QQmlComponent bg_comp(engine, QUrl("qrc:/background_demo.qml"));
		qInfo() << bg_comp.errors();

		QQmlComponent top_comp(engine, QUrl("qrc:/toppanel_demo.qml"));
		qInfo() << top_comp.errors();

		QQmlComponent bot_comp(engine, QUrl("qrc:/bottompanel_demo.qml"));
		qInfo() << bot_comp.errors();

		top = create_component(native, &top_comp, screen, &qobj_top);
		bottom = create_component(native, &bot_comp, screen, &qobj_bottom);
		bg = create_component(native, &bg_comp, screen, &qobj_bg);
	} else {
		QQmlComponent bg_comp(engine, QUrl("qrc:/background.qml"));
		qInfo() << bg_comp.errors();

		QQmlComponent top_comp(engine, QUrl("qrc:/toppanel.qml"));
		qInfo() << top_comp.errors();

		QQmlComponent bot_comp(engine, QUrl("qrc:/bottompanel.qml"));
		qInfo() << bot_comp.errors();

		top = create_component(native, &top_comp, screen, &qobj_top);
		bottom = create_component(native, &bot_comp, screen, &qobj_bottom);
		bg = create_component(native, &bg_comp, screen, &qobj_bg);
	}

	if (!screen_name)
		screen = qApp->primaryScreen();
	else
		screen = find_screen(screen_name);

	if (!screen) {
		qDebug() << "No outputs present in the system.";
		return;
	}

	qDebug() << "found primary screen " << qApp->primaryScreen()->name() <<
		"first screen " << qApp->screens().first()->name();
	output = getWlOutput(native, screen);

	/* engine.rootObjects() works only if we had a load() */
	StatusBarModel *statusBar = qobj_top->findChild<StatusBarModel *>("statusBar");
	if (statusBar) {
		qDebug() << "got statusBar objectname, doing init()";
		statusBar->init(engine->rootContext());
	}

	agl_shell_set_panel(agl_shell, top, output, AGL_SHELL_EDGE_TOP);
	agl_shell_set_panel(agl_shell, bottom, output, AGL_SHELL_EDGE_BOTTOM);
	qDebug() << "Setting homescreen to screen  " << screen->name();

	agl_shell_set_background(agl_shell, bg, output);

	/* Delay the ready signal until after Qt has done all of its own setup
	 * in a.exec() */
	QTimer::singleShot(500, [agl_shell](){
		agl_shell_ready(agl_shell);
	});
}

int main(int argc, char *argv[])
{
	setenv("QT_QPA_PLATFORM", "wayland", 1);
	setenv("QT_QUICK_CONTROLS_STYLE", "AGL", 1);

	QGuiApplication app(argc, argv);
	const char *screen_name;
	bool is_demo_val = false;
	struct shell_data shell_data = { nullptr, nullptr };

	QPlatformNativeInterface *native = qApp->platformNativeInterface();
	screen_name = getenv("HOMESCREEN_START_SCREEN");

	const char *is_demo = getenv("HOMESCREEN_DEMO_CI");
	if (is_demo && strcmp(is_demo, "1") == 0)
		is_demo_val = true;

	QCoreApplication::setOrganizationDomain("LinuxFoundation");
	QCoreApplication::setOrganizationName("AutomotiveGradeLinux");
	QCoreApplication::setApplicationName("HomeScreen");
	QCoreApplication::setApplicationVersion("0.7.0");

	// we need to have an app_id
	app.setDesktopFileName("homescreen");

	register_agl_shell(native, &shell_data);
	if (!shell_data.shell) {
		fprintf(stderr, "agl_shell extension is not advertised. "
			"Are you sure that agl-compositor is running?\n");
		exit(EXIT_FAILURE);
	}
	if (!shell_data.shell_desktop) {
		fprintf(stderr, "agl_shell_desktop extension is not advertised. "
			"Are you sure that agl-compositor is running?\n");
		exit(EXIT_FAILURE);
	}

	std::shared_ptr<struct agl_shell> agl_shell{shell_data.shell, agl_shell_destroy};
	Shell *aglShell = new Shell(agl_shell, &app);

	// Import C++ class to QML
	qmlRegisterType<StatusBarModel>("HomeScreen", 1, 0, "StatusBarModel");
	qmlRegisterType<MasterVolume>("MasterVolume", 1, 0, "MasterVolume");

	ApplicationLauncher *launcher = new ApplicationLauncher();
	launcher->setCurrent(QStringLiteral("launcher"));
	HomescreenHandler* homescreenHandler = new HomescreenHandler(aglShell, launcher);

	agl_shell_desktop_add_listener(shell_data.shell_desktop, &shell_desktop_listener, homescreenHandler);

	QQmlApplicationEngine engine;
	QQmlContext *context = engine.rootContext();

	context->setContextProperty("homescreenHandler", homescreenHandler);
	context->setContextProperty("launcher", launcher);
	context->setContextProperty("weather", new Weather());
	context->setContextProperty("bluetooth", new Bluetooth(false, context));

	// We add it here even if we don't use it
	context->setContextProperty("shell", aglShell);

	// Instead of loading main.qml we load one-by-one each of the QMLs,
	// divided now between several surfaces: panels, background.
	load_agl_shell_app(native, &engine, shell_data.shell, screen_name, is_demo_val);

	return app.exec();
}

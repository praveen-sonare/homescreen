// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright (c) 2017, 2018, 2019 TOYOTA MOTOR CORPORATION
 * Copyright (c) 2022 Konsulko Group
 */

#include <QGuiApplication>
#include <QFileInfo>
#include <functional>

#include "homescreenhandler.h"
#include "hmi-debug.h"

QScreen *find_screen(const char *output);

// defined by meson build file
#include QT_QPA_HEADER

// LAUNCHER_APP_ID shouldn't be started by applaunchd as it is started as
// a user session by systemd
#define LAUNCHER_APP_ID          "launcher"

static struct wl_output *
getWlOutput(QPlatformNativeInterface *native, QScreen *screen);

HomescreenHandler::HomescreenHandler(Shell *_aglShell, ApplicationLauncher *launcher, QObject *parent) :
	QObject(parent),
	aglShell(_aglShell)
{
	mp_launcher = launcher;
	mp_applauncher_client = new AppLauncherClient();
	QPlatformNativeInterface *native = qApp->platformNativeInterface();

	//
	// The "started" event is received any time a start request is made to applaunchd,
	// and the application either starts successfully or is already running. This
	// effectively acts as a "switch to app X" action.
	//
	connect(mp_applauncher_client,
		&AppLauncherClient::appStatusEvent,
		this,
		&HomescreenHandler::processAppStatusEvent);
	activateApp(QString("dashboard"));
}

HomescreenHandler::~HomescreenHandler()
{
	delete mp_applauncher_client;
}

static struct wl_output *
getWlOutput(QPlatformNativeInterface *native, QScreen *screen)
{
	void *output = native->nativeResourceForScreen("output", screen);
	return static_cast<struct ::wl_output*>(output);
}

void HomescreenHandler::tapShortcut(QString app_id)
{
	HMI_DEBUG("HomeScreen","tapShortcut %s", app_id.toStdString().c_str());

	if (app_id == LAUNCHER_APP_ID) {
		activateApp(app_id);
		return;
	}

	if (!mp_applauncher_client->startApplication(app_id)) {
		HMI_ERROR("HomeScreen","Unable to start application '%s'",
			  app_id.toStdString().c_str());
		return;
	}
}

/*
 * Keep track of currently running apps and the order in which
 * they were activated. That way, when an app is closed, we can
 * switch back to the previously active one.
 */
void HomescreenHandler::addAppToStack(const QString& app_id)
{
	if (app_id == "homescreen" || app_id == "dashboard")
		return;

	if (!apps_stack.contains(app_id)) {
		apps_stack << app_id;
	} else {
		int current_pos = apps_stack.indexOf(app_id);
		int last_pos = apps_stack.size() - 1;

		if (current_pos != last_pos)
			apps_stack.move(current_pos, last_pos);
	}
}

void HomescreenHandler::activateApp(const QString& app_id)
{
	struct agl_shell *agl_shell = aglShell->shell.get();
	QPlatformNativeInterface *native = qApp->platformNativeInterface();
	struct wl_output *mm_output = getWlOutput(native, qApp->screens().first());

	if (mp_launcher) {
		mp_launcher->setCurrent(app_id);
	}
	HMI_DEBUG("HomeScreen", "Activating app_id %s by default output %p\n",
			app_id.toStdString().c_str(), mm_output);

	// search for a pending application which might have a different output
	auto iter = pending_app_list.begin();
	bool found_pending_app = false;
	while (iter != pending_app_list.end()) {
		const QString &app_to_search = iter->first;

		if (app_to_search == app_id) {
			found_pending_app = true;
			break;
		}

		iter++;
	}

	if (found_pending_app) {
		const QString &output_name = iter->second;
		QScreen *screen =
			::find_screen(output_name.toStdString().c_str());

		mm_output = getWlOutput(native, screen);
		pending_app_list.erase(iter);

		HMI_DEBUG("HomeScreen", "For application %s found another "
				"output to activate %s\n",
				app_id.toStdString().c_str(),
				output_name.toStdString().c_str());
	}

	HMI_DEBUG("HomeScreen", "Activating application %s",
			app_id.toStdString().c_str());

	agl_shell_activate_app(agl_shell, app_id.toStdString().c_str(), mm_output);
}

void HomescreenHandler::deactivateApp(const QString& app_id)
{
	if (apps_stack.contains(app_id)) {
		apps_stack.removeOne(app_id);
		if (!apps_stack.isEmpty())
			activateApp(apps_stack.last());
	}
}

void HomescreenHandler::processAppStatusEvent(const QString &app_id, const QString &status)
{
	HMI_DEBUG("HomeScreen", "Processing application %s, status %s",
			app_id.toStdString().c_str(), status.toStdString().c_str());

	if (status == "started") {
		activateApp(app_id);
	} else if (status == "terminated") {
		HMI_DEBUG("HomeScreen", "Application %s terminated, activating last app", app_id.toStdString().c_str());
		deactivateApp(app_id);
	} else if (status == "deactivated") {
		HMI_DEBUG("HomeScreen", "Application %s deactivated, activating last app", app_id.toStdString().c_str());
	}
}

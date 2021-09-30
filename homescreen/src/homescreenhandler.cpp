/*
 * Copyright (c) 2017, 2018, 2019 TOYOTA MOTOR CORPORATION
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
#include <QFileInfo>
#include "homescreenhandler.h"
#include <functional>

#include <qpa/qplatformnativeinterface.h>

void* HomescreenHandler::myThis = 0;

HomescreenHandler::HomescreenHandler(Shell *_aglShell, ApplicationLauncher *launcher, QObject *parent) :
	QObject(parent),
	aglShell(_aglShell)
{
	mp_launcher = launcher;
}

HomescreenHandler::~HomescreenHandler()
{
}

void HomescreenHandler::init()
{
}

static struct wl_output *
getWlOutput(QPlatformNativeInterface *native, QScreen *screen)
{
	void *output = native->nativeResourceForScreen("output", screen);
	return static_cast<struct ::wl_output*>(output);
}

void HomescreenHandler::tapShortcut(QString application_id)
{
	struct agl_shell *agl_shell = aglShell->shell.get();
	QPlatformNativeInterface *native = qApp->platformNativeInterface();
	struct wl_output *output = getWlOutput(native, qApp->screens().first());

	// start the application here, or do in the WM!

	// this works (and it is redundant the first time), due to the default
	// policy engine installed which actives the application, when starting
	// the first time. Later calls to HomescreenHandler::tapShortcut will
	// require calling 'agl_shell_activate_app'
	agl_shell_activate_app(agl_shell, application_id.toStdString().c_str(), output);

	// keep this for having animations working
	if (mp_launcher) {
		mp_launcher->setCurrent(application_id);
	}
}

void HomescreenHandler::onRep_static(struct json_object* reply_contents)
{
	(void) reply_contents;
}

void HomescreenHandler::onEv_static(const string& event, struct json_object* event_contents)
{
	(void) event;
	(void) event_contents;
}

void HomescreenHandler::onRep(struct json_object* reply_contents)
{
	(void) reply_contents;
}

void HomescreenHandler::onEv(const string& event, struct json_object* event_contents)
{
	(void) event;
	(void) event_contents;
}

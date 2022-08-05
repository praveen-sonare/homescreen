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
#include "hmi-debug.h"

#include <qpa/qplatformnativeinterface.h>

#define APPLAUNCH_DBUS_IFACE     "org.automotivelinux.AppLaunch"
#define APPLAUNCH_DBUS_OBJECT    "/org/automotivelinux/AppLaunch"
/* LAUNCHER_APP_ID shouldn't be started by applaunchd as it is started as a
 * user session by systemd */
#define LAUNCHER_APP_ID          "launcher"

void* HomescreenHandler::myThis = 0;

HomescreenHandler::HomescreenHandler(Shell *_aglShell, ApplicationLauncher *launcher, QObject *parent) :
    QObject(parent),
    aglShell(_aglShell)
{
    mp_launcher = launcher;
    applaunch_iface = new org::automotivelinux::AppLaunch(APPLAUNCH_DBUS_IFACE, APPLAUNCH_DBUS_OBJECT,
                                                          QDBusConnection::sessionBus(), this);
}

HomescreenHandler::~HomescreenHandler()
{
}

void HomescreenHandler::init(void)
{
    myThis = this;

    /*
     * The "started" signal is received any time a start request is made to applaunchd,
     * and the application either starts successfully or is already running. This
     * effectively acts as a "switch to app X" action.
     */
    connect(applaunch_iface, SIGNAL(started(QString)), this, SLOT(appStarted(QString)));
    connect(applaunch_iface, SIGNAL(terminated(QString)), this, SLOT(appTerminated(QString)));
    connect(applaunch_iface, SIGNAL(splited(QString, uint32_t)), this, SLOT(appSplit(QString, uint32_t)));

}

static struct wl_output *
getWlOutput(QPlatformNativeInterface *native, QScreen *screen)
{
	void *output = native->nativeResourceForScreen("output", screen);
	return static_cast<struct ::wl_output*>(output);
}

void HomescreenHandler::tapShortcut(QString application_id)
{
    QDBusPendingReply<> reply;
    HMI_DEBUG("HomeScreen","tapShortcut %s", application_id.toStdString().c_str());

    if (application_id == LAUNCHER_APP_ID)
        goto activate_app;

    reply = applaunch_iface->start(application_id);
    reply.waitForFinished();

    if (reply.isError()) {
        HMI_ERROR("HomeScreen","Unable to start application '%s': %s",
            application_id.toStdString().c_str(),
            reply.error().message().toStdString().c_str());
        return;
    }

activate_app:
    if (mp_launcher) {
        mp_launcher->setCurrent(application_id);
    }
    appStarted(application_id);
}

/*
 * Keep track of currently running apps and the order in which
 * they were activated. That way, when an app is closed, we can
 * switch back to the previously active one.
 */
void HomescreenHandler::addAppToStack(const QString& application_id)
{
    if (application_id == "homescreen")
        return;

    if (!apps_stack.contains(application_id)) {
        apps_stack << application_id;
    } else {
        int current_pos = apps_stack.indexOf(application_id);
        int last_pos = apps_stack.size() - 1;

        if (current_pos != last_pos)
            apps_stack.move(current_pos, last_pos);
    }
}

void HomescreenHandler::appSplit(const QString& application_id, uint32_t orientation)
{
    struct agl_shell *agl_shell = aglShell->shell.get();
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    struct wl_output *output = getWlOutput(native, qApp->screens().first());

    HMI_DEBUG("HomeScreen", "Split application %s", application_id.toStdString().c_str());

    fprintf(stderr, "homescreen: doing a split for app %s with orientation passed %d\n",
		    application_id.toStdString().c_str(), orientation);
    agl_shell_set_app_split(agl_shell, application_id.toStdString().c_str(),
                            orientation, output);
}

void HomescreenHandler::appStarted(const QString& application_id)
{
    struct agl_shell *agl_shell = aglShell->shell.get();
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    struct wl_output *output = getWlOutput(native, qApp->screens().first());

    HMI_DEBUG("HomeScreen", "Activating application %s", application_id.toStdString().c_str());
    agl_shell_activate_app(agl_shell, application_id.toStdString().c_str(), output);
    addAppToStack(application_id);
}

void HomescreenHandler::appTerminated(const QString& application_id)
{
    HMI_DEBUG("HomeScreen", "Application %s terminated, activating last app", application_id.toStdString().c_str());
    if (apps_stack.contains(application_id)) {
        apps_stack.removeOne(application_id);
        if (!apps_stack.isEmpty())
            appStarted(apps_stack.last());
    }
}

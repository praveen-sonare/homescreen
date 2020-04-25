/*
 * Copyright (c) 2019 Collabora Ltd.
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
#include <QDebug>
#include "shell.h"
#include <qpa/qplatformnativeinterface.h>
#include <stdio.h>

static struct wl_output *
getWlOutput(QPlatformNativeInterface *native, QScreen *screen)
{
	void *output = native->nativeResourceForScreen("output", screen);
	return static_cast<struct ::wl_output*>(output);
}

void Shell::activate_app(QWindow *win, const QString &app_id)
{
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    QScreen *screen = win->screen();

    struct wl_output *output = getWlOutput(native, screen);

    qDebug() << "++ activating app_id " << app_id.toStdString().c_str();

    agl_shell_activate_app(this->shell.get(),
                           app_id.toStdString().c_str(),
                           output);
}

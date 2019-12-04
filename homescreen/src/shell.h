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

#ifndef SHELLHANDLER_H
#define SHELLHANDLER_H

#include <QObject>
#include <QString>
#include <QScreen>
#include <QWindow>
#include <memory>
#include "wayland-agl-shell-client-protocol.h"

/*
 * Basic type to wrap the agl_shell wayland object into a QObject, so that it
 * can be used in callbacks from QML.
 */

class Shell : public QObject
{
    Q_OBJECT
    std::shared_ptr<struct agl_shell> shell;
public:
    Shell(std::shared_ptr<struct agl_shell> shell, QObject *parent = nullptr) :
        QObject(parent), shell(shell) 
    {}
public slots:
    void activate_app(QWindow *win, const QString &app_id);
};

#endif // SHELLHANDLER_H

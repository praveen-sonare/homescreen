/*
 * Copyright (c) 2017 TOYOTA MOTOR CORPORATION
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

#ifndef HOMESCREENHANDLER_H
#define HOMESCREENHANDLER_H

#include <QObject>

#include <libhomescreen.hpp>

#include "shell.h"
#include <string>

using namespace std;

class HomescreenHandler : public QObject
{
    Q_OBJECT
public:
    explicit HomescreenHandler(Shell *aglShell, QObject *parent = 0);
    ~HomescreenHandler();

    void init(int port, const char* token);

    Q_INVOKABLE void tapShortcut(QString application_id);

    void onRep(struct json_object* reply_contents);
    void onEv(const string& event, struct json_object* event_contents);

    static void* myThis;
    static void onRep_static(struct json_object* reply_contents);
    static void onEv_static(const string& event, struct json_object* event_contents);

signals:
    void showNotification(QString application_id, QString icon_path, QString text);
    void showInformation(QString info);
private:
    LibHomeScreen *mp_hs;
    Shell *aglShell;
};

#endif // HOMESCREENHANDLER_H

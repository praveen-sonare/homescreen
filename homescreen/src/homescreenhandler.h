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
#include <qlibhomescreen.h>
#include <libwindowmanager.h>
#include <string>

using namespace std;

class QQmlApplicationEngine;

class HomescreenHandler : public QObject
{
    Q_OBJECT
public:
    enum CHANGE_LAYOUT_PATTERN {
        P_LEFT_METER_RIGHT_MAP = 0,
        P_LEFT_MAP_RIGHT_METER
    };
    Q_ENUMS(CHANGE_LAYOUT_PATTERN)
    explicit HomescreenHandler(QObject *parent = 0);
    ~HomescreenHandler();

    void init(const char* role, int port, const char* token);
    void attach(QQmlApplicationEngine* engine);
    void setWMHandler(WMHandler &handler);

    Q_INVOKABLE void tapShortcut(QString application_name);
    Q_INVOKABLE void changeLayout(int pattern);
    Q_INVOKABLE void reboot();

    void onRep(struct json_object* reply_contents);
    void onEv(const string& event, struct json_object* event_contents);

    static void* myThis;
    static void onRep_static(struct json_object* reply_contents);
    static void onEv_static(const string& event, struct json_object* event_contents);

signals:
    void notification(QString id, QString icon, QString text);
    void information(QString text);

private Q_SLOTS:
    void disconnect_frame_swapped(void);

private:
    QLibHomeScreen *mp_qhs;
    LibWindowmanager *mp_wm;
    std::string m_role;
    QMetaObject::Connection loading;
};

#endif // HOMESCREENHANDLER_H

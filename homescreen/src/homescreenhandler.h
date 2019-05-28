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
#include <QQuickWindow>
#include <qlibhomescreen.h>
#include <qlibwindowmanager.h>
#include <string>

using namespace std;

class HomescreenHandler : public QObject
{
    Q_OBJECT
public:
    explicit HomescreenHandler(QObject *parent = 0);
    ~HomescreenHandler();

    void init(int port, const char* token, QLibWindowmanager *qwm, QString myname);

    Q_INVOKABLE void tapShortcut(QString application_name, bool is_full);
    Q_INVOKABLE QString getCurrentApplication();
    Q_INVOKABLE void killRunningApplications();
    Q_INVOKABLE void reboot();
    void setCurrentApplication(QString application_name);
    int getPidOfApplication(QString application_name);

    void onRep(struct json_object* reply_contents);
    void onEv(const string& event, struct json_object* event_contents);

    static void* myThis;
    static void onRep_static(struct json_object* reply_contents);
    static void onEv_static(const string& event, struct json_object* event_contents);
    void setQuickWindow(QQuickWindow *qw);

signals:
    void showNotification(QString application_id, QString icon_path, QString text);
    void showInformation(QString info);
    void shortcutChanged(QString shortcut_id, QString shortcut_name, QString position);
    void showWindow();
    void hideWindow();

public slots:
    void updateShortcut(QString id, struct json_object* object);

private:
    QLibHomeScreen *mp_qhs;
    QLibWindowmanager *mp_qwm;
    QString m_myname;
    QString current_application;
};

#endif // HOMESCREENHANDLER_H

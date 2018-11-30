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

#include "homescreenhandler.h"
#include <functional>
#include <QQmlApplicationEngine>
#include <QtQuick/QQuickWindow>
#include "hmi-debug.h"

void* HomescreenHandler::myThis = 0;

HomescreenHandler::HomescreenHandler(QObject *parent) :
    QObject(parent),
    mp_hs(NULL), mp_wm(NULL), m_role()
{

}

HomescreenHandler::~HomescreenHandler()
{
    if (mp_hs != NULL) {
        delete mp_hs;
    }
    if (mp_wm != NULL) {
        delete mp_wm;
    }
}

void HomescreenHandler::init(const char* role, int port, const char *token)
{
    this->m_role = role;

    // LibWindowManager initialize
    mp_wm = new LibWindowmanager();
    if(mp_wm->init(port,token) != 0){
        exit(EXIT_FAILURE);
    }

    int surface = mp_wm->requestSurface(m_role.c_str());
    if (surface < 0) {
        exit(EXIT_FAILURE);
    }
    std::string ivi_id = std::to_string(surface);
    setenv("QT_IVI_SURFACE_ID", ivi_id.c_str(), true);

    // LibHomeScreen initialize
    mp_hs = new LibHomeScreen();
    mp_hs->init(port, token);

    myThis = this;

    mp_hs->registerCallback(nullptr, HomescreenHandler::onRep_static);

    mp_hs->set_event_handler(LibHomeScreen::Event_OnScreenMessage, [this](json_object *object){
        const char *display_message = json_object_get_string(
            json_object_object_get(object, "display_message"));
        HMI_DEBUG("HomeScreen","set_event_handler Event_OnScreenMessage display_message = %s", display_message);
    });
}

void HomescreenHandler::setWMHandler(WMHandler& h) {
    h.on_sync_draw = [&](const char* role, const char* area, Rect r) {
        this->mp_wm->endDraw(this->m_role.c_str());
    };
    mp_wm->setEventHandler(h);
}

void HomescreenHandler::disconnect_frame_swapped(void)
{
    qDebug("Let's start homescreen");
    QObject::disconnect(this->loading);
    mp_wm->activateWindow(m_role.c_str(), "fullscreen");
}

void HomescreenHandler::attach(QQmlApplicationEngine* engine)
{
    QQuickWindow *window = qobject_cast<QQuickWindow *>(engine->rootObjects().first());
    this->loading = QObject::connect(window, SIGNAL(frameSwapped()), this, SLOT(disconnect_frame_swapped()));
}

void HomescreenHandler::changeLayout(int pattern)
{
    HMI_NOTICE("HomeScreen", "Pressed %d, %s", pattern,
        (pattern == P_LEFT_METER_RIGHT_MAP) ? "left:meter, right:map": "left:map, right:meter");
    ChangeAreaReq req;
    std::unordered_map<std::string, Rect> map_list;
    switch(pattern) {
        case P_LEFT_METER_RIGHT_MAP:
            map_list["split.main"] = Rect(0, 0, 1280, 720);
            map_list["split.sub"] = Rect(1280, 0, 640, 720);
            break;
        case P_LEFT_MAP_RIGHT_METER:
            map_list["split.main"] = Rect(640, 0, 1280, 720);
            map_list["split.sub"] = Rect(0, 0, 640, 720);
            break;
        default:
            break;
    }
    if(map_list.size() != 0)
    {
        req.setAreaReq(map_list);
        HMI_NOTICE("Homescreen", "Change layout");
        mp_wm->changeAreaSize(req);
    }
}

void HomescreenHandler::tapShortcut(QString application_name)
{
    HMI_DEBUG("HomeScreen","tapShortcut %s", application_name.toStdString().c_str());
    mp_hs->tapShortcut(application_name.toStdString().c_str());
}

void HomescreenHandler::onRep_static(struct json_object* reply_contents)
{
    static_cast<HomescreenHandler*>(HomescreenHandler::myThis)->onRep(reply_contents);
}

void HomescreenHandler::onEv_static(const string& event, struct json_object* event_contents)
{
    static_cast<HomescreenHandler*>(HomescreenHandler::myThis)->onEv(event, event_contents);
}

void HomescreenHandler::onRep(struct json_object* reply_contents)
{
    const char* str = json_object_to_json_string(reply_contents);
    HMI_DEBUG("HomeScreen","HomeScreen onReply %s", str);
}

void HomescreenHandler::onEv(const string& event, struct json_object* event_contents)
{
    const char* str = json_object_to_json_string(event_contents);
    HMI_DEBUG("HomeScreen","HomeScreen onEv %s, contents: %s", event.c_str(), str);

    if (event.compare("homescreen/on_screen_message") == 0) {
        struct json_object *json_data = json_object_object_get(event_contents, "data");
        struct json_object *json_display_message = json_object_object_get(json_data, "display_message");
        const char* display_message = json_object_get_string(json_display_message);

        HMI_DEBUG("HomeScreen","display_message = %s", display_message);
    }
}

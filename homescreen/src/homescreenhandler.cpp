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

#include <QFileInfo>
#include "homescreenhandler.h"
#include <functional>
#include "hmi-debug.h"

void* HomescreenHandler::myThis = 0;

HomescreenHandler::HomescreenHandler(QObject *parent) :
    QObject(parent),
    mp_hs(NULL)
{

}

HomescreenHandler::~HomescreenHandler()
{
    if (mp_hs != NULL) {
        delete mp_hs;
    }
}

void HomescreenHandler::init(int port, const char *token)
{
    mp_hs = new LibHomeScreen();
    mp_hs->init(port, token);

    myThis = this;

    mp_hs->registerCallback(nullptr, HomescreenHandler::onRep_static);

    mp_hs->set_event_handler(LibHomeScreen::Event_OnScreenMessage, [this](json_object *object){
        const char *display_message = json_object_get_string(
            json_object_object_get(object, "display_message"));
        HMI_DEBUG("HomeScreen","set_event_handler Event_OnScreenMessage display_message = %s", display_message);
    });

    mp_hs->set_event_handler(LibHomeScreen::Event_ShowNotification,[this](json_object *object){
       json_object *p_obj = json_object_object_get(object, "parameter");
       const char *icon = json_object_get_string(
                   json_object_object_get(p_obj, "icon"));
       const char *text = json_object_get_string(
                   json_object_object_get(p_obj, "text"));
       const char *app_id = json_object_get_string(
                   json_object_object_get(p_obj, "caller"));
       HMI_DEBUG("HomeScreen","Event_ShowNotification icon=%s, text=%s, caller=%s", icon, text, app_id);
       QFileInfo icon_file(icon);
       QString icon_path;
       if (icon_file.isFile() && icon_file.exists()) {
           icon_path = QString(QLatin1String(icon));
       } else {
           icon_path = "./images/Utility_Logo_Grey-01.svg";
       }

       emit showNotification(QString(QLatin1String(app_id)), icon_path, QString(QLatin1String(text)));
    });

    mp_hs->set_event_handler(LibHomeScreen::Event_ShowInformation,[this](json_object *object){
       json_object *p_obj = json_object_object_get(object, "parameter");
       const char *info = json_object_get_string(
                   json_object_object_get(p_obj, "info"));

       emit showInformation(QString(QLatin1String(info)));
    });
}

void HomescreenHandler::tapShortcut(QString application_id)
{
    HMI_DEBUG("HomeScreen","tapShortcut %s", application_id.toStdString().c_str());
    struct json_object* j_json = json_object_new_object();
    struct json_object* value;
    value = json_object_new_string("normal");
    json_object_object_add(j_json, "area", value);

    mp_hs->showWindow(application_id.toStdString().c_str(), j_json);
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

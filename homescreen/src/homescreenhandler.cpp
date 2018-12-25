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
#include <QProcess>
#include <dirent.h>
#include <stdio.h>
#include "hmi-debug.h"

#define BUF_SIZE                        1024

void* HomescreenHandler::myThis = 0;

HomescreenHandler::HomescreenHandler(QObject *parent) :
    QObject(parent),
    mp_hs(NULL),
    current_applciation("launcher")
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
}

void HomescreenHandler::tapShortcut(QString application_name, bool is_full)
{
    HMI_DEBUG("HomeScreen","tapShortcut %s", application_name.toStdString().c_str());
    struct json_object* j_json = json_object_new_object();
    struct json_object* value;
    if(is_full) {
        value = json_object_new_string("fullscreen");
        HMI_DEBUG("HomeScreen","ullscreen");
    } else {
        value = json_object_new_string("normal");
        HMI_DEBUG("HomeScreen","normal");
    }
    json_object_object_add(j_json, "area", value);
    mp_hs->showWindow(application_name.toStdString().c_str(), j_json);
}

void HomescreenHandler::setCurrentApplication(QString application_name)
{
    HMI_DEBUG("HomeScreen","setCurrentApplication %s", application_name.toStdString().c_str());
    current_applciation = application_name;
}

QString HomescreenHandler::getCurrentApplication()
{
    HMI_DEBUG("HomeScreen","getCurrentApplication %s", current_applciation.toStdString().c_str());
    return current_applciation;
}

int HomescreenHandler::getPidOfApplication(QString application_name) {
    DIR *dir = NULL;
    struct dirent *dir_ent_ptr = NULL;
    FILE *fp = NULL;
    char file_path[50] = {0};
    char cur_task_ame[50] = {0};
    char buf[BUF_SIZE] = {0};
    int pid = -1;

    dir = opendir("/proc");
    if (dir) {
        while((dir_ent_ptr = readdir(dir)) != NULL) {
            if ((strcmp(dir_ent_ptr->d_name, ".") == 0) || (strcmp(dir_ent_ptr->d_name, "..") == 0)
                || (DT_DIR != dir_ent_ptr->d_type))
                continue;
            sprintf(file_path, "/proc/%s/status", dir_ent_ptr->d_name);
            fp = fopen(file_path, "r");
            if (fp) {
                if (fgets(buf, BUF_SIZE - 1, fp) == NULL) {
                    fclose(fp);
                    continue;
                }
                sscanf(buf, "%*s %s", cur_task_ame);
                if (0 == strcmp(application_name.toStdString().c_str(), cur_task_ame)) {
                    pid = atoi(dir_ent_ptr->d_name);
                    break;
                }
            }
        }
    }

    return pid;
}

void HomescreenHandler::killRunningApplications()
{
    QProcess *proc = new QProcess;
    QProcess *proc2 = new QProcess;
//    int num = getPidOfApplication("afbd-video@0.1");
//    QString procNum = QString::number(num);
    QString command = "/usr/bin/pkill videoplayer";
    QString command2 = "/usr/bin/pkill navigation";
    proc->start(command);
    proc2->start(command2);
    HMI_DEBUG("homescreen", command.toStdString().c_str());
    HMI_DEBUG("homescreen", command2.toStdString().c_str());
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

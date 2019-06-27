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

#include <QFileInfo>
#include "homescreenconnect.h"
#include <functional>
#include <QProcess>
#include <dirent.h>
#include <stdio.h>
#include <qstring.h>
#include <thread>
#include "hmi-debug.h"
#include <unistd.h>
#include <qdebug.h>

static const char API[] = "alexa-voiceagent";
static const char CARLACLIENTAPI[] = "carlaclient";
const string vshl_core_event = "{\"events\": []}";
const string vshl_core_refreshevent = "{\"refresh_token\": \"ws\"}";

const std::vector<std::string> HomescreenConnect::event_lists {
    std::string("alexa-voiceagent/voice_cbl_codepair_received_event"),
};

static void _on_hangup_static(void *closure, struct afb_wsj1 *wsj)
{
    static_cast<HomescreenConnect*>(closure)->on_hangup(NULL,wsj);
}

static void _on_call_static(void *closure, const char *api, const char *verb, struct afb_wsj1_msg *msg)
{
    /* HomescreenConnect is not called from other process */
}

static void _on_event_static(void* closure, const char* event, struct afb_wsj1_msg *msg)
{
    static_cast<HomescreenConnect*>(closure)->on_event(NULL,event,msg);
}

static void _on_reply_static(void *closure, struct afb_wsj1_msg *msg)
{
    static_cast<HomescreenConnect*>(closure)->on_reply(NULL,msg);
}

static void event_loop_run(struct sd_event* loop){
    sd_event_loop(loop);
    sd_event_unref(loop);
}

HomescreenConnect::HomescreenConnect(QObject *parent) :
    QObject(parent)
{
    timer = new QTimer(this);
}

HomescreenConnect::~HomescreenConnect()
{
    if(sp_websock != NULL)
    {
        afb_wsj1_unref(sp_websock);
    }
    if(mploop)
    {
        sd_event_exit(mploop, 0);
    }
}

int HomescreenConnect::init(int port, const string& token)
{
    int ret = 0;

    connect(timer,SIGNAL(timeout()),this,SLOT(subscribe()));
    connect(this,SIGNAL(stopTimer()),this,SLOT(stopGetCode()));
    if(port > 0 && token.size() > 0)
    {
        mport = port;
        mtoken = token;
    }
    else
    {
        HMI_ERROR("HomescreenConnect","port and token should be > 0, Initial port and token uses.");
    }

    ret = initialize_websocket();
    if(ret != 0 )
    {
        HMI_ERROR("HomescreenConnect","Failed to initialize websocket");
    }
    else{

        HMI_DEBUG("HomescreenConnect","Initialized");
        timer->setSingleShot(true);
        timer->start(3000);
    }

    return ret;
}

int HomescreenConnect::initialize_websocket(void)
{
    HMI_DEBUG("HomescreenConnect"," initialize_websocket called");
    mploop = NULL;
    int ret = sd_event_new(&mploop);
    if(ret < 0)
    {
        HMI_ERROR("HomescreenConnect","Failed to create event loop");
        return -1;
    }

    {
        // enforce context to avoid initialization/goto error
        std::thread th(event_loop_run, mploop);
        th.detach();
    }

    /* Initialize interface from websocket */
    minterface.on_hangup = _on_hangup_static;
    minterface.on_call = _on_call_static;
    minterface.on_event = _on_event_static;
    muri += "ws://localhost:" + to_string(mport) + "/api?token=" + mtoken; /*To be modified*/
    sp_websock = afb_ws_client_connect_wsj1(mploop, muri.c_str(), &minterface, this);

    if(sp_websock == NULL)
    {
        HMI_ERROR("HomescreenConnect","Failed to create websocket connection");
       return -1;
    }

    return 0;
}

void HomescreenConnect::subscribe(void)
{
    HMI_DEBUG("HomescreenConnect"," subscribe called");
    if(!sp_websock)
    {
        return;
    }

    json_object* j_obj = json_tokener_parse(vshl_core_event.c_str());

    int ret = afb_wsj1_call_j(sp_websock, API, "subscribeToCBLEvents", j_obj, _on_reply_static, this);
    if (ret < 0) {
        HMI_ERROR("HomescreenConnect","Failed to call subscribeToCBLEvents verb");
    }

    json_object* j_obj1 = json_tokener_parse(vshl_core_refreshevent.c_str());
    ret = afb_wsj1_call_j(sp_websock, API, "setRefreshToken", j_obj1, _on_reply_static, this);
    if (ret < 0) {
        HMI_ERROR("HomescreenConnect","Failed to call setRefreshToken verb");
    }

    HMI_DEBUG("HomescreenConnect"," subscribe OK");
    return;
}

/************* Callback Function *************/

void HomescreenConnect::on_hangup(void *closure, struct afb_wsj1 *wsj)
{
    HMI_DEBUG("HomescreenConnect"," on_hangup called");
}

void HomescreenConnect::on_call(void *closure, const char *api, const char *verb, struct afb_wsj1_msg *msg)
{
    HMI_DEBUG("HomescreenConnect"," on_call called");
}

/*
* event is like "homescreen/hvac"
* msg is like {"event":"homescreen\/hvac","data":{"type":"tap_shortcut"},"jtype":"afb-event"}
* so you can get
    event name : struct json_object obj = json_object_object_get(msg,"event")
*/
void HomescreenConnect::on_event(void *closure, const char *event, struct afb_wsj1_msg *msg)
{
    HMI_DEBUG("HomescreenConnect","on_event event: (%s) msg: (%s).", event, afb_wsj1_msg_object_s(msg));

    if (strstr(event, API) == NULL) {
        return;
    }

    struct json_object* ev_contents = afb_wsj1_msg_object_j(msg);
    struct json_object *json_event_str;

    if(!json_object_object_get_ex(ev_contents, "event", &json_event_str)) {
        HMI_ERROR("HomescreenConnect", "got json_event_str error.");
        return;
    }
     const char* eventinfo = json_object_get_string(json_event_str);
    if(strcasecmp(eventinfo, HomescreenConnect::event_lists[0].c_str()) == 0){
        struct json_object *json_data_str;
        if(!json_object_object_get_ex(ev_contents, "data", &json_data_str)) {
            HMI_ERROR("HomescreenConnect", "got json_data_str error.");
            return;
        }

        struct json_object *json_payload;
        if(!json_object_object_get_ex(json_data_str, "payload", &json_payload)) {
            HMI_ERROR("HomescreenConnect", "got json_payload error.");
            return;
        }

        struct json_object *json_code;
        struct json_object *json_data = json_tokener_parse(json_object_get_string(json_payload));
        const char* code = json_object_get_string(json_data);
        QString str = QString::fromLocal8Bit(code);
        const char* warnginfo = "Connectting to Alexa.";
        int index = str.indexOf("\"code\"");
        if( index != -1 ){
            QString codestr1 = str.right(str.length()-index-7);
            const QString pos = "\"";
            int index1 = codestr1.indexOf(pos);
            if( index1 != -1 ){
                QString codestr2 = codestr1.right(codestr1.length()-index1-1);
                QString codestr = codestr2.left(6);
                emit showInformation(QString(QLatin1String(warnginfo)));
                std::string str = codestr.toStdString();
                if(send_code(str.c_str())){
                     HMI_ERROR("HomescreenConnect", "send_code error.");
                }
                emit stopTimer();
            }
        }
    }
}

int HomescreenConnect::send_code(const char *str)
{
    HMI_DEBUG("HomescreenConnect"," SendCode,%s",str);
    struct json_object* obj = json_object_new_object();
    json_object_object_add(obj, "amazon_code", json_object_new_string(str));

    int ret = afb_wsj1_call_j(sp_websock, CARLACLIENTAPI, "set_amazon_code", obj, _on_reply_static, this);
    if (ret < 0) {
          HMI_ERROR("HomescreenConnect","Failed to call set_amazon_code verb");
    }

    return ret;
}

void HomescreenConnect::stopGetCode(void)
{
    timer->stop();
}

/**
 * msg is like ({"response":{"verb":"subscribe","error":0},"jtype":"afb-reply","request":{"status":"success","info":"homescreen binder subscribe event name [on_screen_message]"}})
 * msg is like ({"response":{"verb":"tap_shortcut","error":0},"jtype":"afb-reply","request":{"status":"success","info":"afb_event_push event [tap_shortcut]"}})
 */
void HomescreenConnect::on_reply(void *closure, struct afb_wsj1_msg *msg)
{
    HMI_DEBUG("HomescreenConnect"," on_reply called,%s",afb_wsj1_msg_object_s(msg));
}

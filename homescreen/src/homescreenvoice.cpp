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
#include "homescreenvoice.h"
#include <functional>
#include <QProcess>
#include <dirent.h>
#include <stdio.h>
#include <thread>
#include "hmi-debug.h"



static const char API[] = "vshl-core";
const string vshl_core_event = "{\"va_id\": \"VA-001\", \"events\": [\"voice_dialogstate_event\"]}";

static void _on_hangup_static(void *closure, struct afb_wsj1 *wsj)
{
    static_cast<HomescreenVoice*>(closure)->on_hangup(NULL,wsj);
}

static void _on_call_static(void *closure, const char *api, const char *verb, struct afb_wsj1_msg *msg)
{
    /* HomescreenVoice is not called from other process */
}

static void _on_event_static(void* closure, const char* event, struct afb_wsj1_msg *msg)
{
    static_cast<HomescreenVoice*>(closure)->on_event(NULL,event,msg);
}

static void _on_reply_static(void *closure, struct afb_wsj1_msg *msg)
{
    static_cast<HomescreenVoice*>(closure)->on_reply(NULL,msg);
}

const std::vector<std::string> HomescreenVoice::state_lists {
    std::string("IDLE"),
    std::string("LISTENING"),
    std::string("THINKING"),
    std::string("UNKNOWN"),
    std::string("SPEAKING")
};


static void event_loop_run(struct sd_event* loop){
    sd_event_loop(loop);
    sd_event_unref(loop);
}

HomescreenVoice::HomescreenVoice(QObject *parent) :
    QObject(parent)
{

}

HomescreenVoice::~HomescreenVoice()
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

int HomescreenVoice::init(int port, const string& token)
{
    int ret = 0;
    if(port > 0 && token.size() > 0)
    {
        mport = port;
        mtoken = token;
    }
    else
    {
        HMI_ERROR("HomescreenVoice","port and token should be > 0, Initial port and token uses.");
    }

    ret = initialize_websocket();
    if(ret != 0 )
    {
        HMI_ERROR("HomescreenVoice","Failed to initialize websocket");
    }
    else{
        HMI_DEBUG("HomescreenVoice","Initialized");
        subscribe(vshl_core_event);
    }

    return ret;
}

int HomescreenVoice::initialize_websocket(void)
{
    HMI_DEBUG("HomescreenVoice"," initialize_websocket called");
    mploop = NULL;
    int ret = sd_event_new(&mploop);
    if(ret < 0)
    {
        HMI_ERROR("HomescreenVoice","Failed to create event loop");
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
        HMI_ERROR("HomescreenVoice","Failed to create websocket connection");
       return -1;
    }

    return 0;
}

int HomescreenVoice::subscribe(const string event_name)
{
    HMI_DEBUG("HomescreenVoice"," subscribe called");
    if(!sp_websock)
    {
        return -1;
    }

    json_object* j_obj = json_tokener_parse(event_name.c_str());

    int ret = afb_wsj1_call_j(sp_websock, API, "subscribe", j_obj, _on_reply_static, this);
    if (ret < 0) {
        HMI_ERROR("HomescreenVoice","Failed to call verb");
    }
    return ret;
}

int HomescreenVoice::startListening(void)
{
    HMI_DEBUG("HomescreenVoice"," startListening called");
    struct json_object* j_obj = json_object_new_object();
    int ret = afb_wsj1_call_j(sp_websock, API, "startListening", j_obj, _on_reply_static, this);
    if (ret < 0) {
        HMI_ERROR("HomescreenVoice"," startListening Failed to call verb");
    }
    return ret;
}

/************* Callback Function *************/

void HomescreenVoice::on_hangup(void *closure, struct afb_wsj1 *wsj)
{
    HMI_DEBUG("HomescreenVoice"," on_hangup called");
}

void HomescreenVoice::on_call(void *closure, const char *api, const char *verb, struct afb_wsj1_msg *msg)
{
    HMI_DEBUG("HomescreenVoice"," on_call called");
}

/*
* event is like "homescreen/hvac"
* msg is like {"event":"homescreen\/hvac","data":{"type":"tap_shortcut"},"jtype":"afb-event"}
* so you can get
    event name : struct json_object obj = json_object_object_get(msg,"event")
*/
void HomescreenVoice::on_event(void *closure, const char *event, struct afb_wsj1_msg *msg)
{
    HMI_DEBUG("HomescreenVoice","event: (%s) msg: (%s).", event, afb_wsj1_msg_object_s(msg));

    if (strstr(event, API) == NULL) {
        return;
    }
    struct json_object* ev_contents = afb_wsj1_msg_object_j(msg);
    struct json_object *json_data_str;
    if(!json_object_object_get_ex(ev_contents, "data", &json_data_str)) {
        HMI_ERROR("HomescreenVoice", "got ev_contents error.");
        return;
    }

    struct json_object *json_state;
    struct json_object *json_data = json_tokener_parse(json_object_get_string(json_data_str));
    if(!json_object_object_get_ex(json_data, "state", &json_state)) {
        HMI_ERROR("HomescreenVoice", "got json_data1 error.");
        return;
    }

    const char* corestatus = json_object_get_string(json_state);

    if (strcasecmp(corestatus, HomescreenVoice::state_lists[0].c_str()) == 0) {
        emit statusChanged(true);
    }
    else if ((strcasecmp(corestatus, HomescreenVoice::state_lists[1].c_str()) == 0)||
             (strcasecmp(corestatus, HomescreenVoice::state_lists[2].c_str()) == 0)||
             (strcasecmp(corestatus, HomescreenVoice::state_lists[3].c_str()) == 0)||
             (strcasecmp(corestatus, HomescreenVoice::state_lists[4].c_str()) == 0)){
        emit statusChanged(false);
    }
}

/**
 * msg is like ({"response":{"verb":"subscribe","error":0},"jtype":"afb-reply","request":{"status":"success","info":"homescreen binder subscribe event name [on_screen_message]"}})
 * msg is like ({"response":{"verb":"tap_shortcut","error":0},"jtype":"afb-reply","request":{"status":"success","info":"afb_event_push event [tap_shortcut]"}})
 */
void HomescreenVoice::on_reply(void *closure, struct afb_wsj1_msg *msg)
{
    HMI_DEBUG("HomescreenVoice"," on_reply called");
}

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

#ifndef HOMESCREENVOICE_H
#define HOMESCREENVOICE_H

#include <QObject>
#include <qlibwindowmanager.h>
#include <string>
#include <functional>
#include <json-c/json.h>
#include <systemd/sd-event.h>
extern "C"
{
#include <afb/afb-wsj1.h>
#include <afb/afb-ws-client.h>
}

using namespace std;

class HomescreenVoice : public QObject
{
    Q_OBJECT
public:
    explicit HomescreenVoice(QObject *parent = 0);
    ~HomescreenVoice();
    static const std::vector<std::string> state_lists;

    int init(int port, const string& token);
    void on_hangup(void *closure, struct afb_wsj1 *wsj);
    void on_call(void *closure, const char *api, const char *verb, struct afb_wsj1_msg *msg);
    void on_event(void *closure, const char *event, struct afb_wsj1_msg *msg);
    void on_reply(void *closure, struct afb_wsj1_msg *msg);
    Q_INVOKABLE int startListening(void);
signals:
    void statusChanged(bool status);

private:
    int initialize_websocket(void);
    int subscribe(const string event_name);

    struct afb_wsj1* sp_websock;
    struct afb_wsj1_itf minterface;
    sd_event* mploop;
    std::string muri;
    int mport = 2000;
    std::string mtoken = "hs";

};

#endif // HOMESCREENVOICE_H

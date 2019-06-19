/*
 * Copyright (c) 2017 Panasonic Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef GST_WAYLAND_HPP
#define GST_WAYLAND_HPP

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include <gio/gio.h>

#include <ilm/ilm_control.h>
#include <ilm/ilm_input.h>

#include <libwindowmanager.h>
#include <libhomescreen.hpp>

#define AGL_FATAL(fmt, ...) fatal("ERROR: " fmt "\n", ##__VA_ARGS__)
#define AGL_WARN(fmt, ...) warn("WARNING: " fmt "\n", ##__VA_ARGS__)
#define AGL_DEBUG(fmt, ...) debug("DEBUG: " fmt "\n", ##__VA_ARGS__)
#define AGL_TRACE(file,line) debug("%s:%d\n", file,line);

void fatal (const char* format, ...);
void warn (const char* format, ...);
void debug (const char* format, ...);

struct Carla_Info {
  std::string yaw;
  double longitude;
  double latitude;
  int speed;
};

class ILMControl
{
  public:
    ILMControl(notificationFunc callback, void *user_data) {
        ilm_init();
        ilm_registerNotification(callback, user_data);
    }

    ~ILMControl(void) {
        ilm_unregisterNotification();
        ilm_destroy();
        AGL_DEBUG("ilm_destory().\n");
    }
};

class RunXDG
{
  public:
    RunXDG(int port, const char* token, const char* id);

    void start(void);
    void notify_ivi_control_cb(ilmObjectType object, t_ilm_uint id,
                               t_ilm_bool created);
    static void notify_ivi_control_cb_static (ilmObjectType object,
                                              t_ilm_uint id,
                                              t_ilm_bool created,
                                              void *user_data);
    void on_event(void *closure, const char *event, struct afb_wsj1_msg *msg);

  private:
    std::string m_role;
    std::string m_path;

    std::string m_id;
    pid_t m_pid;

    int m_port;
    std::string m_token;

    struct afb_wsj1* sp_websock;
    struct afb_wsj1_itf minterface;
    sd_event* mploop;
    std::string muri;
    struct Carla_Info m_carla_info; 

    LibWindowmanager *m_wm;
    LibHomeScreen *m_hs;
    ILMControl *m_ic;

    t_ilm_surface m_ivi_id;

    std::map<int, int> m_surfaces;  // pair of <afm:rid, ivi:id>

    bool m_pending_create = false;

    int init_wm(void);
    int init_hs(void);
    int init_carlaclient(void);

    int parse_config(const char *file);

    void setup_ivi_surface(void);
};

#endif  // GST_WAYLAND_HPP

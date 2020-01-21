/*
 * Copyright (c) 2020 Collabora Ltd.
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

#ifndef PWS_HEADER_H
#define PWS_HEADER_H

#include <systemd/sd-event.h>

/* linker will complain it can't find them and given they're only for C */
extern "C" {
#include "afb/afb-ws-client.h"
#include "afb/afb-proto-ws.h"
}

#define TIMEOUT_SD_LOOP	30000000

struct pws_data_source_json_reply;

struct pws_data_source {
	struct sd_event *loop;
	struct afb_proto_ws *pws;
	int callcount;
	int num_id;	/* key id */

	struct json_object *reply;
	bool reply_valid;
};

void
pws_data_source_reply_destroy(struct pws_data_source *pws);

/* inits a connection to connect_to
 *
 */
struct pws_data_source *
pws_data_source_init(const char *connect_to);

/* destroys connection init'ed by pws_data_source_init
 */
void
pws_data_source_destroy(struct pws_data_source *pws);


/*
 * starts the app by @afm_name
 *
 * returns 0 in case of success, negative in case of failure
 *
 */
int
pws_start_process(struct pws_data_source *pws, const char *afm_name);

/*
 * terminates the app by @afm_name
 *
 * returns true case of success, false in case of failure
 */
bool
pws_stop_process(struct pws_data_source *pws, const char *afm_name);

/* returns pid or -1 in case the @afm_name is not running
 *
 */
int
pws_check_process_is_running(struct pws_data_source *pws, const char *afm_name);

/* returns #no of (current) active, running applications
 *
 * (prints) as well
 */
size_t
pws_list_runners(struct pws_data_source *pws);

/* returns #no of (all) applications
 *
 * (prints) as well
 */
size_t
pws_list_runnables(struct pws_data_source *pws);

/*
 */
size_t
pws_get_list_runnables(struct pws_data_source *pws, struct json_object **json_obj);

#endif

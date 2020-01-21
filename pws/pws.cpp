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

#include <cstdio>
#include <cstdlib>

#include <systemd/sd-event.h>
#include <json-c/json.h>
#include <assert.h>
#include "pws.h"

#if !defined(JSON_C_TO_STRING_NOSLASHESCAPE)
#define JSON_C_TO_STRING_NOSLASHESCAPE 0
#endif

#define PWS_DEBUG

static void
idle(struct pws_data_source *pws_d_source)
{
	struct sd_event *loop = pws_d_source->loop;

	for (;;) {
		if (!pws_d_source->callcount)
			break;

		sd_event_run(loop, TIMEOUT_SD_LOOP);
	}
}

static void
dec_callcount(struct pws_data_source *pws_d_source)
{
	if (!pws_d_source)
		return;

	pws_d_source->callcount--;
}

static void
inc_callcount(struct pws_data_source *pws_d_source)
{
	if (!pws_d_source)
		return;

	pws_d_source->callcount++;
}

static void
on_pws_reply(void *closure, void *request, struct json_object *result,
	     const char *error, const char *info)
{
	struct pws_data_source *pws_d_source =
		static_cast<struct pws_data_source *>(closure);

	assert(pws_d_source != NULL);

#ifdef PWS_DEBUG
	fprintf(stdout, "ON-REPLY %s: %s %s\n%s\n", (char*) request, error,
			info ?: "",
			json_object_to_json_string_ext(result,
				JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_NOSLASHESCAPE));

	fflush(stdout);
#endif

	/* in case of an error do not set the reply */
	if (!info && !error) {
		/* should be cleaned-up after a proper request */
		assert(pws_d_source->reply_valid == false);

		pws_d_source->reply = result;
		pws_d_source->reply_valid = true;
	} else {
		fprintf(stdout, "ON-REPLY: err: %s, request: %s, info %s\n",
				error, (char *) request, info);
	}

	/* necessary when getting the reply to exit idle() */
	dec_callcount(pws_d_source);
}

static void
on_pws_event_create(void *closure, uint16_t event_id, const char *event_name)
{
	struct pws_data_source *pws_d_source =
		static_cast<struct pws_data_source *>(closure);
	(void) pws_d_source;

#ifdef PWS_DEBUG
	fprintf(stdout, "ON-EVENT-CREATE: [%d:%s]\n", event_id, event_name);
	fflush(stdout);
#endif
}

static void
on_pws_event_remove(void *closure, uint16_t event_id)
{
	struct pws_data_source *pws_d_source =
		static_cast<struct pws_data_source *>(closure);

	(void) pws_d_source;
#ifdef PWS_DEBUG
	fprintf(stdout, "ON-EVENT-REMOVE: [%d]\n", event_id);
	fflush(stdout);
#endif
}

static void
on_pws_event_subscribe(void *closure, void *request, uint16_t event_id)
{
	struct pws_data_source *pws_d_source =
		static_cast<struct pws_data_source *>(closure);

	(void) pws_d_source;
#ifdef PWS_DEBUG
	fprintf(stdout, "ON-EVENT-SUBSCRIBE %s: [%d]\n", (char*)request, event_id);
	fflush(stdout);
#endif
}

static void
on_pws_event_unsubscribe(void *closure, void *request, uint16_t event_id)
{
	struct pws_data_source *pws_d_source =
		static_cast<struct pws_data_source *>(closure);

	(void) pws_d_source;
#ifdef PWS_DEBUG
	fprintf(stdout, "ON-EVENT-UNSUBSCRIBE %s: [%d]\n", (char*)request, event_id);
	fflush(stdout);
#endif
}

static void
on_pws_event_push(void *closure, uint16_t event_id, struct json_object *data)
{
	struct pws_data_source *pws_d_source =
		static_cast<struct pws_data_source *>(closure);

	(void) pws_d_source;
#ifdef PWS_DEBUG
	fprintf(stdout, "ON-EVENT-PUSH: [%d]\n%s\n",
			event_id,
			json_object_to_json_string_ext(data, JSON_C_TO_STRING_NOSLASHESCAPE));
	fprintf(stdout, "ON-EVENT-PUSH: [%d]\n%s\n",
			event_id,
			json_object_to_json_string_ext(data, JSON_C_TO_STRING_PRETTY |
							     JSON_C_TO_STRING_NOSLASHESCAPE));

	fflush(stdout);
#endif
}

static void
on_pws_event_broadcast(void *closure, const char *event_name,
		      struct json_object *data,
		      const afb_proto_ws_uuid_t uuid, uint8_t hop)
{
	struct pws_data_source *pws_d_source =
		static_cast<struct pws_data_source *>(closure);

	(void) pws_d_source;
	(void) uuid;
	(void) hop;

#ifdef PWS_DEBUG
	fprintf(stdout, "ON-EVENT-BROADCAST: [%s]\n%s\n",
			event_name,
			json_object_to_json_string_ext(data, JSON_C_TO_STRING_NOSLASHESCAPE));
	fprintf(stdout, "ON-EVENT-BROADCAST: [%s]\n%s\n",
			event_name,
			json_object_to_json_string_ext(data, JSON_C_TO_STRING_PRETTY |
							     JSON_C_TO_STRING_NOSLASHESCAPE));
	fflush(stdout);
#endif
}

/* called when pws hangsup */
static void
on_pws_hangup(void *closure)
{
	struct pws_data_source *pws_d_source =
		static_cast<struct pws_data_source *>(closure);

	(void) pws_d_source;
#ifdef PWS_DEBUG
	printf("ON-HANGUP\n");
	fflush(stdout);
#endif

	exit(EXIT_FAILURE);
}

/* makes a call */
static int
pws_call(const char *verb, const char *object,
	 struct pws_data_source *pws_d_source)
{
	char *key;
	struct json_object *o;
	enum json_tokener_error jerr;

	struct afb_proto_ws *pws = pws_d_source->pws;
	int rc;

	assert(pws != NULL);

	/* allocates an id for the request */
	rc = asprintf(&key, "%d:%s", ++pws_d_source->num_id, verb);
	if (rc < 0)
		return -1;

	/* echo the command if asked */
	fprintf(stdout, "SEND-CALL: %s %s\n", verb, object?:"null");

	inc_callcount(pws_d_source);

	if (object == NULL || object[0] == 0) {
		o = NULL;
	} else {
		o = json_tokener_parse_verbose(object, &jerr);
		if (jerr != json_tokener_success)
			o = json_object_new_string(object);
	}

	/* send the request */
	rc = afb_proto_ws_client_call(pws, verb, o, 0, 0, key, NULL);
	json_object_put(o);
	if (rc < 0) {
		fprintf(stderr, "calling %s(%s) failed: %m\n", verb, object ?: "");
		dec_callcount(pws_d_source);
		free(key);
		return -1;
	}

	free(key);
	return 0;
}

/* the callback interface for pws */
static struct afb_proto_ws_client_itf pws_itf = {
	.on_reply = on_pws_reply,
	.on_event_create = on_pws_event_create,
	.on_event_remove = on_pws_event_remove,
	.on_event_subscribe = on_pws_event_subscribe,
	.on_event_unsubscribe = on_pws_event_unsubscribe,
	.on_event_push = on_pws_event_push,
	.on_event_broadcast = on_pws_event_broadcast,
};

struct pws_data_source *
pws_data_source_init(const char *connect_to)
{
	struct afb_proto_ws *pws;
	struct sd_event *sd_loop;
	struct pws_data_source *pws_d_source = nullptr;

	if (!connect_to) {
		fprintf(stderr, "Failed to get a connect_to\n");
		return nullptr;
	}

	if (sd_event_default(&sd_loop) < 0) {
		fprintf(stderr, "Failed to get a default event\n");
		return nullptr;
	}

	pws_d_source =
		static_cast<struct pws_data_source *>(calloc(1, sizeof(struct pws_data_source)));

	if (!pws_d_source) {
		fprintf(stderr, "Failed to allocate memory\n");
		return nullptr;
	}

	pws = afb_ws_client_connect_api(sd_loop, connect_to,
					&pws_itf, pws_d_source);
	if (!pws) {
		fprintf(stderr, "Failed to create a afb_proto_ws\n");
		return nullptr;
	}

	afb_proto_ws_on_hangup(pws, on_pws_hangup);

	pws_d_source->pws = pws;
	pws_d_source->loop = sd_loop;
	pws_d_source->callcount = 0;
	pws_d_source->num_id = 0;

	pws_d_source->reply = nullptr;
	pws_d_source->reply_valid = false;

	return pws_d_source;
}

static int
pws_do_call(struct pws_data_source *pws_d_source,
	    const char *verb, const char *object)
{
	if (!verb || !object || !pws_d_source)
		return -1;

	if (pws_call(verb, object, pws_d_source) < 0)
		return -1;

	idle(pws_d_source);

	return 0;
}

void
pws_data_source_destroy(struct pws_data_source *pws_d_source)
{
	assert(pws_d_source != nullptr);

	sd_event_unref(pws_d_source->loop);
	free(pws_d_source);
}

void
pws_data_source_reply_destroy(struct pws_data_source *pws)
{
	assert(pws->reply_valid == true);
	pws->reply_valid = false;
	free(pws->reply);
}


int
pws_start_process(struct pws_data_source *pws, const char *afm_name)
{
	int rid = -1;
	if (pws_do_call(pws, "start", afm_name) < 0)
		return -1;

	if (pws->reply_valid) {
		struct json_object *result = pws->reply;

		rid = json_object_get_int(result);
		fprintf(stdout, "ON-REPLY %s\n",
				json_object_to_json_string_ext(result,
					JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_NOSLASHESCAPE));

		pws_data_source_reply_destroy(pws);
	}

	return rid;
}

bool
pws_stop_process(struct pws_data_source *pws, const char *afm_name)
{
	bool term = false;
	if (pws_do_call(pws, "terminate", afm_name) < 0)
		return term;

	if (pws->reply_valid) {
		struct json_object *result = pws->reply;

		term = json_object_get_boolean(result);
#ifdef PWS_DEBUG
		fprintf(stdout, "ON-REPLY %s\n",
				json_object_to_json_string_ext(result,
				JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_NOSLASHESCAPE));
#endif
		pws_data_source_reply_destroy(pws);
	}

	return term;
}


int
pws_check_process_is_running(struct pws_data_source *pws, const char *afm_name)
{
	int rid = -1;

	if (pws_do_call(pws, "state", afm_name) < 0)
		return rid;

	if (pws->reply_valid) {
		struct json_object *result = pws->reply;
		struct json_object *obj;

		json_object_object_get_ex(result, "runid", &obj);
		if (obj) {
			rid = json_object_get_int(obj);
			fprintf(stdout, "Found rid %d\n", rid);
		}
#ifdef PWS_DEBUG
		fprintf(stdout, "ON-REPLY: %s\n",
				json_object_to_json_string_ext(result,
				JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_NOSLASHESCAPE));
#endif
		pws_data_source_reply_destroy(pws);
	}

	return rid;
}

size_t
pws_list_runners(struct pws_data_source *pws)
{
	size_t items = 0;
	if (pws_do_call(pws, "runners", "true") < 0)
		return items;

	if (pws->reply_valid) {
		struct json_object *result = pws->reply;

		/* at least one is running */
		items = json_object_array_length(result);

#ifdef PWS_DEBUG
		fprintf(stdout, "found %ld items\n", items);
		fprintf(stdout, "ON-REPLY: %s\n",
				json_object_to_json_string_ext(result,
				JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_NOSLASHESCAPE));
#endif
		pws_data_source_reply_destroy(pws);
	}

	return items;
}

size_t
pws_list_runnables(struct pws_data_source *pws)
{
	size_t items = 0;
	if (pws_do_call(pws, "runnables", "true") < 0)
		return items;

	if (pws->reply_valid) {
		struct json_object *result = pws->reply;

		items = json_object_array_length(result);
#ifdef PWS_DEBUG
		fprintf(stdout, "found %ld items\n", items);
		fprintf(stdout, "ON-REPLY: %s\n",
				json_object_to_json_string_ext(result,
				JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_NOSLASHESCAPE));
#endif
		pws_data_source_reply_destroy(pws);
	}

	return items;
}

/*
 * need to free(json_string) once you're done with it
 *
 * call pws_data_source_reply_destroy(pws) once you're done with it.
 */
size_t
pws_get_list_runnables(struct pws_data_source *pws,  struct json_object **json)
{
	size_t items = 0;
	if (pws_do_call(pws, "runnables", "true") < 0)
		return items;

	fprintf(stdout, "pws_get_list_runnables()\n");

	if (pws->reply_valid) {
		struct json_object *result = pws->reply;

		items = json_object_array_length(result);
#ifdef PWS_DEBUG
		fprintf(stdout, "found %ld items\n", items);
		fprintf(stdout, "ON-REPLY: %s\n",
				json_object_to_json_string_ext(result,
				JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_NOSLASHESCAPE));
#endif
		if (items == 0) {
			fprintf(stdout, "pws_get_list_runnables() turn items %d, bails sooner\n", items);
			*json = NULL;
			return items;
		}

		fprintf(stdout, "pws_get_list_runnables() json reply is set\n");
		*json = result;
	}

	fprintf(stdout, "pws_get_list_runnables() turn items %d\n", items);
	return items;
}

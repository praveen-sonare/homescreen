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

#include <json-c/json.h>

#include "launcher.h"
#include "pws.h"

int
Launcher::setup_pws_connection(void)
{
	pws = pws_data_source_init(afm_sock_name.toStdString().c_str());
	if (!pws)
		return -1;

	connection_set = true;
	return 0;
}

bool
Launcher::connection_is_set(void)
{
	return connection_set;
}

int
Launcher::start(const QString &app)
{
	int rid = -1;

	if (!connection_set)
		return rid;

	rid = pws_start_process(pws, app.toStdString().c_str());
	if (rid > 0)
		applications.insert(app, rid);

	return rid;
}

bool
Launcher::terminate(const QString &app)
{
	if (!connection_set)
		return -1;

	if (pws_stop_process(pws, app.toStdString().c_str()))
		return true;

	return false;
}

bool
Launcher::is_running(const QString &app)
{
	int rid = -1;

	if (!connection_set)
		return false;

	rid = pws_check_process_is_running(pws, app.toStdString().c_str());
	/* remove it from QHash if it was there and current no longer shows up */
	if (rid > 0) {
		return true;
	} else {
		if (applications.contains(app))
			applications.remove(app);
	}

	return false;
}

size_t
Launcher::get_list_runnables(QString *qstr)
{
	size_t items = 0;
	struct json_object *json;

	if (!connection_set)
		return false;

	items = pws_get_list_runnables(pws, &json);
	if (json)
		*qstr = QString(json_object_to_json_string(json));
	else
		*qstr = nullptr;

	/* necessary as pws_get_list_runnables won't free() the json reply on
	 * its own */
	pws_data_source_reply_destroy(pws);

	return items;
}

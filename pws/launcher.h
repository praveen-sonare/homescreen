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

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QObject>
#include <QString>
#include <QScreen>
#include <QWindow>
#include <QHash>

#include <memory>

#include "pws.h"

class Launcher : public QObject
{
	Q_OBJECT

public:
	Launcher(const QString &sock_name, QObject *parent = nullptr) :
		QObject(parent), afm_sock_name(sock_name)
	{
		pws = nullptr;
		connection_set = false;
	}

	~Launcher()
	{
		destroy();
	}

	// call this before to any start-up
	int setup_pws_connection(void);
	bool connection_is_set(void);

public slots:
	int start(const QString &app);
	bool terminate(const QString &app);
	bool is_running(const QString &app);
	size_t get_list_runnables(QString *data);

private:
	struct pws_data_source *pws;
	bool connection_set;
	QString afm_sock_name;

	QHash<QString, int> applications; // stores the apps started

	void destroy(void)
	{
		pws_data_source_destroy(pws);
	}
};

#endif // LAUNCHER_H

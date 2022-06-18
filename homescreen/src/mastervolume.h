/*
 * Copyright (C) 2017 Konsulko Group
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

#include <QtCore/QObject>
#include <QQmlEngine>
#include "vehiclesignals.h"

class MasterVolume : public QObject
{
	Q_OBJECT
	Q_PROPERTY (uint32_t volume READ getVolume WRITE setVolume NOTIFY VolumeChanged)

private:
	qint32 m_volume;
	VehicleSignals *m_vs;
	bool m_connected;

	void updateVolume(QString value);

public:
	MasterVolume(QObject* parent = nullptr);
	~MasterVolume() = default;

	Q_INVOKABLE qint32 getVolume() const;
	Q_INVOKABLE void setVolume(qint32 val);

private slots:
	void onConnected();
	void onAuthorized();
	void onDisconnected();
	void onGetSuccessResponse(QString path, QString value, QString timestamp);
	void onSignalNotification(QString path, QString value, QString timestamp);

signals:
	void VolumeChanged();
};

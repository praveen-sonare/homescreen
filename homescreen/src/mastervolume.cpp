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

#include "mastervolume.h"
#include <QJsonObject>
#include <QTimer>
#include <QtDebug>

MasterVolume::MasterVolume(QObject* parent)
	: QObject(parent)
	, m_volume{50}
{
	connect(&m_client, SIGNAL(connected()), this, SLOT(onClientConnected()));
	connect(&m_client, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
	connect(&m_client, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onClientError(QAbstractSocket::SocketError)));
	connect(&m_client, SIGNAL(eventReceived(QString, const QJsonValue&)), this, SLOT(onClientEventReceived(QString, const QJsonValue&)));
}

void MasterVolume::open(const QUrl& url)
{
	m_url = url;
	TryOpen();
}

qint32 MasterVolume::getVolume() const
{
	return m_volume;
}

void MasterVolume::setVolume(qint32 volume)
{
	if (m_volume != volume)
	{
		m_volume = volume;
		QJsonObject arg;
		arg.insert("action", "volume");
		arg.insert("value", volume);
		m_client.call("ahl-4a", "activerole", arg, [](bool, const QJsonValue&) {
			// Nothing to do, events will update sliders
		});
	}
}

void MasterVolume::onClientConnected()
{
	// Subscribe to 4a events
	m_client.call("ahl-4a", "subscribe", QJsonValue(), [this](bool r, const QJsonValue&) {
		if (r) qDebug() << "MasterVolume::onClientConnected - subscribed to 4a events!";
		else qCritical () << "MasterVolume::onClientConnected - Failed to subscribe to 4a events!";
	});
}

void MasterVolume::onClientDisconnected()
{
	qDebug() << "MasterVolume::onClientDisconnected!";
	QTimer::singleShot(1000, this, SLOT(TryOpen()));
}

void MasterVolume::onClientError(QAbstractSocket::SocketError se)
{
	qDebug() << "MasterVolume::onClientError: " << se;
}

void MasterVolume::onClientEventReceived(QString name, const QJsonValue& data)
{
	qDebug() << "MasterVolume::onClientEventReceived[" << name << "]: " << data;
	if (name == "ahl-4a/volume_changed")
	{
		QJsonObject arg = data.toObject();
		bool active = arg["active"].toBool();
		if (active)
		{
			// QString role = arg["role"].toString();
			int volume = arg["volume"].toInt();
			if (m_volume != volume)
			{
				m_volume = volume;
				emit VolumeChanged();
			}
		}
	}
}

void MasterVolume::TryOpen()
{
	m_client.open(m_url);
}

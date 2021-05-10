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

#define MASTER_CONTROL "Master Playback"

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
		arg.insert("control", MASTER_CONTROL);
		double v = (double) volume / 100.0;
		arg.insert("value", v);
		m_client.call("audiomixer", "volume", arg);
	}
}

void MasterVolume::onClientConnected()
{
	QJsonObject arg;
	arg.insert("control", MASTER_CONTROL);
	m_client.call("audiomixer", "volume", arg, [this](bool r, const QJsonValue& v) {
		if (r && v.isObject()) {
			int volume = v.toObject()["response"].toObject()["volume"].toDouble() * 100;
			volume = qBound(0, volume, 100);
			if (m_volume != volume)
			{
				m_volume = volume;
				emit VolumeChanged();
			}
		}

		QJsonObject arg;
		arg.insert("event", "volume_changed");
		m_client.call("audiomixer", "subscribe", arg);
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
	if (name == "audiomixer/volume_changed")
	{
		QString ctlName = data.toObject()["control"].toString();

		if (ctlName != MASTER_CONTROL)
			return;

		int volume = data.toObject()["value"].toDouble() * 100;
		volume = qBound(0, volume, 100);
		if (m_volume != volume)
		{
			m_volume = volume;
			emit VolumeChanged();
		}
	}
}

void MasterVolume::TryOpen()
{
	m_client.open(m_url);
}

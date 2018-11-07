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

#include <pulse/pulseaudio.h>

class MasterVolume : public QObject
{
	Q_OBJECT
		Q_PROPERTY (uint32_t volume READ getVolume WRITE setVolume NOTIFY volumeChanged)

	public:
		MasterVolume(QObject *parent = 0)
			: QObject(parent), m_volume(32768)
		{
		}

		~MasterVolume() {}

		uint32_t getVolume() const { return m_volume; }
		void setVolume(pa_volume_t volume);

	public slots:
		void changeExternalVolume(int volume);

	signals:
		void volumeChanged(void);
		void sliderVolumeChanged(int volume_delta);
		void externalVolumeChanged(uint32_t volume);

	private:
		uint32_t m_volume;
};

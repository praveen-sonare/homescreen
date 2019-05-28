/*
 * Copyright (C) 2016 The Qt Company Ltd.
 * Copyright (C) 2016, 2017 Mentor Graphics Development (Deutschland) GmbH
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

#include "applicationlauncher.h"

#include "afm_user_daemon_proxy.h"

#include "hmi-debug.h"

extern org::AGL::afm::user *afm_user_daemon_proxy;

ApplicationLauncher::ApplicationLauncher(QObject *parent)
    : QObject(parent)
    , m_launching(false)
    , m_timeout(new QTimer(this))
{
    m_timeout->setInterval(3000);
    m_timeout->setSingleShot(true);
    connect(m_timeout, &QTimer::timeout, [&]() {
        setLaunching(false);
    });
    connect(this, &ApplicationLauncher::launchingChanged, [&](bool launching) {
        if (launching)
            m_timeout->start();
        else
            m_timeout->stop();
    });
    connect(this, &ApplicationLauncher::currentChanged, [&]() {
        setLaunching(false);
    });
}

int ApplicationLauncher::launch(const QString &application)
{
    int result = -1;
    HMI_DEBUG("HomeScreen","ApplicationLauncher launch %s.", application.toStdString().c_str());

    result = afm_user_daemon_proxy->start(application).value().toInt();
    HMI_DEBUG("HomeScreen","ApplicationLauncher pid: %d.", result);

    if (result > 1) {
        setLaunching(true);
    }

    return result;
}

bool ApplicationLauncher::isLaunching() const
{
    return m_launching;
}

void ApplicationLauncher::setLaunching(bool launching)
{
    if (m_launching == launching) return;
    m_launching = launching;
    launchingChanged(launching);
}

QString ApplicationLauncher::current() const
{
    return m_current;
}

void ApplicationLauncher::setCurrent(const QString &current)
{
    if (m_current == current) return;
    m_current = current;
    qDebug() << "setCurrent" << m_current;
    emit currentChanged(current);
}

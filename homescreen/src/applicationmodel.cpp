/*
 * Copyright (C) 2016 The Qt Company Ltd.
 * Copyright (C) 2016, 2017 Mentor Graphics Development (Deutschland) GmbH
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

#include "applicationmodel.h"
#include "appinfo.h"

#include <QtCore/QDebug>

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include "afm_user_daemon_proxy.h"

extern org::AGL::afm::user *afm_user_daemon_proxy;

class ApplicationModel::Private
{
public:
    Private();

    QList<AppInfo> data;
};

namespace {
    QString get_icon_name(QJsonObject const &i)
    {
        QString icon = i["id"].toString().split("@").front();
        if (icon == "hvac" || icon == "poi") {
            icon = icon.toUpper();
        } else if (icon == "mediaplayer") {
            icon = "Multimedia";
        } else {
            icon[0] = icon[0].toUpper();
        }
        return icon;
    }
}

ApplicationModel::Private::Private()
{
    QString apps = afm_user_daemon_proxy->runnables(QStringLiteral(""));
    QJsonDocument japps = QJsonDocument::fromJson(apps.toUtf8());
    for (auto const &app : japps.array()) {
        QJsonObject const &jso = app.toObject();
        auto const name = jso["name"].toString();
        auto const id = jso["id"].toString();
        auto const icon = get_icon_name(jso);
        this->data.append(AppInfo(icon, name, id));
        qDebug() << "name:" << name << "icon:" << icon << "id:" << id;
    }
}

ApplicationModel::ApplicationModel(QObject *parent)
    : QAbstractListModel(parent)
    , d(new Private())
{
}

ApplicationModel::~ApplicationModel()
{
    delete this->d;
}

int ApplicationModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return this->d->data.count();
}

QVariant ApplicationModel::data(const QModelIndex &index, int role) const
{
    QVariant ret;
    if (!index.isValid())
        return ret;

    switch (role) {
    case Qt::DecorationRole:
        ret = this->d->data[index.row()].iconPath();
        break;
    case Qt::DisplayRole:
        ret = this->d->data[index.row()].name();
        break;
    case Qt::UserRole:
        ret = this->d->data[index.row()].id();
        break;
    default:
        break;
    }

    return ret;
}

QHash<int, QByteArray> ApplicationModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Qt::DecorationRole] = "icon";
    roles[Qt::DisplayRole] = "name";
    roles[Qt::UserRole] = "id";
    return roles;
}

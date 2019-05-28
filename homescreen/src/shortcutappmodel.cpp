/*
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

#include "shortcutappmodel.h"
#include "hmi-debug.h"
#include <unistd.h>
#define SHORTCUTKEY_PATH "/var/local/lib/afm/applications/homescreen/0.1/etc/registeredApp.json"

class ShortcutAppModel::Private
{
public:
    Private();

    QList<RegisterApp> data;
};

ShortcutAppModel::Private::Private()
{
}


ShortcutAppModel::ShortcutAppModel(QObject *parent)
    : QAbstractListModel(parent)
    , d(new Private())
{
    getAppQueue();
}

ShortcutAppModel::~ShortcutAppModel()
{
    delete this->d;
}

int ShortcutAppModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return this->d->data.count();
}

QVariant ShortcutAppModel::data(const QModelIndex &index, int role) const
{
    QVariant ret;
    if (!index.isValid())
        return ret;

    switch (role) {
    case Qt::DecorationRole:
        ret = this->d->data[index.row()].icon;
        break;
    case Qt::DisplayRole:
        ret = this->d->data[index.row()].name;
        break;
    case Qt::UserRole:
        ret = this->d->data[index.row()].id;
        break;
    default:
        break;
    }

    return ret;
}

QHash<int, QByteArray> ShortcutAppModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Qt::DecorationRole] = "icon";
    roles[Qt::DisplayRole] = "name";
    roles[Qt::UserRole] = "id";
    return roles;
}

void ShortcutAppModel::changeShortcut(QString id, QString name, QString position)
{
    for(int i = 1; i < d->data.size(); i++) {
        if(id == d->data.at(i).id) {
            return;
        }
    }
    d->data.removeAt(position.toInt() + 1);

    RegisterApp temp;
    temp.id = id;
    temp.name = name;
    temp.icon = temp.icon = getIconPath(temp.id);
    if (temp.icon == "") {
        temp.isBlank = true;
    } else {
        temp.isBlank = false;
    }

    d->data.insert(position.toInt() + 1, temp);
    setAppQueue();
    emit updateShortcut();
    struct json_object* obj = makeAppListJson();
    emit shortcutUpdated(QString("launcher"), obj);
}

struct json_object* ShortcutAppModel::makeAppListJson()
{
    struct json_object* obj = json_object_new_object();
    struct json_object* obj_array = json_object_new_array();
    for(int i = 1; i < d->data.size(); i++)
    {
        struct json_object* obj_shortcut = json_object_new_object();
        json_object_object_add(obj_shortcut, "shortcut_id", json_object_new_string(d->data.at(i).id.toStdString().c_str()));
        json_object_object_add(obj_shortcut, "shortcut_name", json_object_new_string(d->data.at(i).name.toStdString().c_str()));
        json_object_array_add(obj_array, obj_shortcut);
    }
    json_object_object_add(obj, "shortcut", obj_array);
    HMI_DEBUG("Homescreen", "makeAppListJson id1=%s",json_object_new_string(d->data.at(1).name.toStdString().c_str()));
    return obj;
}

QString ShortcutAppModel::getId(int index) const
{
    return d->data.at(index).id;
}

QString ShortcutAppModel::getName(int index) const
{
    return d->data.at(index).name;
}

QString ShortcutAppModel::getIcon(int index) const
{
    return d->data.at(index).icon;
}

bool ShortcutAppModel::isBlank(int index) const
{
    return d->data.at(index).isBlank;
}

QString ShortcutAppModel::getIconPath(QString id)
{
    QString name = id.section('@', 0, 0);
    QString version = id.section('@', 1, 1);
    QString boardIconPath = "/var/local/lib/afm/applications/" + name + "/" + version + "/icon.svg";
    QString appIconPath = ":/images/Shortcut/" + name + ".svg";
    if (QFile::exists(boardIconPath)) {
        return "file://" + boardIconPath;
    } else if (QFile::exists(appIconPath)) {
        return appIconPath.section('/', 1, -1);
    }
    return "";
}

void ShortcutAppModel::getAppQueue()
{
    QProcess *process = new QProcess(this);
    if(checkAppFile()) {
        process->start("cp /var/local/lib/afm/applications/homescreen/0.1/etc/registeredApp.aaa.json /var/local/lib/afm/applications/homescreen/0.1/etc/registeredApp.json");
    } else {
        process->start("cp /var/local/lib/afm/applications/homescreen/0.1/etc/registeredApp.json /var/local/lib/afm/applications/homescreen/0.1/etc/registeredApp.aaa.json");
    }
    QThread::msleep(300);

    QFile file(SHORTCUTKEY_PATH);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray allData = file.readAll();
        QString str(allData);
        if(str == "") {
            file.close();

        }
        QJsonParseError json_error;
        QJsonDocument jsonDoc(QJsonDocument::fromJson(allData, &json_error));

        if(json_error.error != QJsonParseError::NoError)
        {
             HMI_ERROR("HomeScreen", "registeredApp.json error");
             return;
        }

        QJsonObject rootObj = jsonDoc.object();

        QJsonObject subObj = rootObj.value("1st shortcut key").toObject();
        setAppQueuePoint(subObj["id"].toString(), subObj["name"].toString());
        subObj = rootObj.value("2nd shortcut key").toObject();
        setAppQueuePoint(subObj["id"].toString(), subObj["name"].toString());
        subObj = rootObj.value("3rd shortcut key").toObject();
        setAppQueuePoint(subObj["id"].toString(), subObj["name"].toString());
        subObj = rootObj.value("4th shortcut key").toObject();
        setAppQueuePoint(subObj["id"].toString(), subObj["name"].toString());
    }
    file.close();
}

void ShortcutAppModel::setAppQueuePoint(QString id, QString name)
{
    app.id = id;
    app.icon = getIconPath(app.id);
    if (app.icon == "") {
        app.isBlank = true;
    } else {
        app.isBlank = false;
    }
    app.name = name;
    d->data.append(app);
}

void ShortcutAppModel::screenUpdated()
{
    struct json_object* obj = makeAppListJson();
    emit shortcutUpdated(QString("launcher"), obj);
}

void ShortcutAppModel::setAppQueue()
{
    QFile file(SHORTCUTKEY_PATH);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonObject rootObj, subObj1, subObj2, subObj3, subObj4;
        subObj1.insert("id", d->data.at(0).id);
        subObj1.insert("name", d->data.at(0).name);
        subObj2.insert("id", d->data.at(1).id);
        subObj2.insert("name", d->data.at(1).name);
        subObj3.insert("id", d->data.at(2).id);
        subObj3.insert("name", d->data.at(2).name);
        subObj4.insert("id", d->data.at(3).id);
        subObj4.insert("name", d->data.at(3).name);
        rootObj.insert("1st shortcut key", subObj1);
        rootObj.insert("2nd shortcut key", subObj2);
        rootObj.insert("3rd shortcut key", subObj3);
        rootObj.insert("4th shortcut key", subObj4);

        QJsonDocument jsonDoc;
        jsonDoc.setObject(rootObj);

        file.write(jsonDoc.toJson());
    } else {
        HMI_ERROR("HomeScreen", "write to registeredApp.json file failed");
    }
    file.flush();
    fsync(file.handle());
    file.close();
}

bool ShortcutAppModel::checkAppFile()
{
    bool fileError = false;
    QFile file(SHORTCUTKEY_PATH);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray line = file.readLine();
        if(line == "\n" || line.isEmpty()) {
            fileError = true;
        }
    } else {
        fileError = true;
        HMI_ERROR("HomeScreen", "registeredApp.json file open failed");
    }
    file.close();
    return fileError;
}

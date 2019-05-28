/*
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

#include <QGuiApplication>
#include <QCommandLineParser>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQml/qqml.h>
#include <QQuickWindow>
#include <QThread>

#include <weather.h>
//#include <bluetooth.h>
#include "applicationlauncher.h"
#include "statusbarmodel.h"
#include "afm_user_daemon_proxy.h"
#include "homescreenhandler.h"
#include "toucharea.h"
#include "hmi-debug.h"
#include <QBitmap>

// XXX: We want this DBus connection to be shared across the different
// QML objects, is there another way to do this, a nice way, perhaps?
org::AGL::afm::user *afm_user_daemon_proxy;

namespace {

struct Cleanup {
    static inline void cleanup(org::AGL::afm::user *p) {
        delete p;
        afm_user_daemon_proxy = Q_NULLPTR;
    }
};

void noOutput(QtMsgType, const QMessageLogContext &, const QString &)
{
}

}

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
    const char* graphic_role = "homescreen";

    // use launch process
    QScopedPointer<org::AGL::afm::user, Cleanup> afm_user_daemon_proxy(new org::AGL::afm::user("org.AGL.afm.user",
                                                                                               "/org/AGL/afm/user",
                                                                                               QDBusConnection::sessionBus(),
                                                                                               0));
    ::afm_user_daemon_proxy = afm_user_daemon_proxy.data();

    QCoreApplication::setOrganizationDomain("LinuxFoundation");
    QCoreApplication::setOrganizationName("AutomotiveGradeLinux");
    QCoreApplication::setApplicationName("HomeScreen");
    QCoreApplication::setApplicationVersion("0.7.0");

    QCommandLineParser parser;
    parser.addPositionalArgument("port", a.translate("main", "port for binding"));
    parser.addPositionalArgument("secret", a.translate("main", "secret for binding"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(a);
    QStringList positionalArguments = parser.positionalArguments();

    int port = 1700;
    QString token = "wm";

    if (positionalArguments.length() == 2) {
        port = positionalArguments.takeFirst().toInt();
        token = positionalArguments.takeFirst();
    }

    HMI_DEBUG("HomeScreen","port = %d, token = %s", port, token.toStdString().c_str());

    // import C++ class to QML
    // qmlRegisterType<ApplicationLauncher>("HomeScreen", 1, 0, "ApplicationLauncher");
    qmlRegisterType<StatusBarModel>("HomeScreen", 1, 0, "StatusBarModel");

    ApplicationLauncher *launcher = new ApplicationLauncher();
    HomescreenHandler* homescreenHandler = new HomescreenHandler();
    homescreenHandler->init(graphic_role, port, token.toStdString().c_str());

    QUrl bindingAddress;
    bindingAddress.setScheme(QStringLiteral("ws"));
    bindingAddress.setHost(QStringLiteral("localhost"));
    bindingAddress.setPort(port);
    bindingAddress.setPath(QStringLiteral("/api"));

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("token"), token);
    bindingAddress.setQuery(query);

    TouchArea* touchArea = new TouchArea();

    // mail.qml loading
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("homescreenHandler", homescreenHandler);
    engine.rootContext()->setContextProperty("touchArea", touchArea);
    engine.rootContext()->setContextProperty("launcher", launcher);
    engine.rootContext()->setContextProperty("weather", new Weather(bindingAddress));
//    engine.rootContext()->setContextProperty("bluetooth", new Bluetooth(bindingAddress));
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    QObject *root = engine.rootObjects().first();

    WMHandler wmh;
    wmh.on_screen_updated = [launcher, root](std::vector<std::string> list) {
        for(const auto& i : list) {
            HMI_DEBUG("HomeScreen", "ids=%s", i.c_str());
        }
        int arrLen = list.size();
        QString label = QString("");
        for( int idx = 0; idx < arrLen; idx++)
        {
            label = list[idx].c_str();
            HMI_DEBUG("HomeScreen","Event_ScreenUpdated application: %s.", label.toStdString().c_str());
            QMetaObject::invokeMethod(launcher, "setCurrent", Qt::QueuedConnection, Q_ARG(QString, label));
            if(label == "launcher") {
                QMetaObject::invokeMethod(root, "turnToNormal");
            } else {
                QMetaObject::invokeMethod(root, "turnToFullscreen");
            }
            if((arrLen == 1) && (QString("restriction") != label)) {
                QMetaObject::invokeMethod(root, "disableSplitSwitchBtn");
            } else {
                QMetaObject::invokeMethod(root, "enableSplitSwitchBtn");
            }
        }
    };
    homescreenHandler->setWMHandler(wmh);
    homescreenHandler->attach(&engine);

    QQuickWindow *window = qobject_cast<QQuickWindow *>(root);

    touchArea->setWindow(window);
    QThread* thread = new QThread;
    touchArea->moveToThread(thread);
    QObject::connect(thread, &QThread::started, touchArea, &TouchArea::init);

    thread->start();

    QList<QObject *> sobjs = engine.rootObjects();
    StatusBarModel *statusBar = sobjs.first()->findChild<StatusBarModel *>("statusBar");
    statusBar->init(bindingAddress, engine.rootContext());

    return a.exec();
}

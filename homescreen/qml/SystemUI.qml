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

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1
import QtWayland.Compositor 1.0
import HomeScreen 1.0
import QtGraphicalEffects 1.0

Window {
    id: root
    visible: true
    flags: Qt.FramelessWindowHint
    width: container.width * container.scale
    height: container.height * container.scale
    title: 'HomeScreen'
    property alias applicationStack: applicationStack

    property var app2item: new Object
    property string appLaunching

    Component {
        id: chrome
        ShellSurfaceItem {
            onSurfaceDestroyed: destroy()
        }
    }

    function show(shellSurface) {
        var a2i = root.app2item
        var item = chrome.createObject(root, {"shellSurface": shellSurface})
        a2i[appLaunching] = item
        root.app2item = a2i
        shellSurface.sendConfigure(Qt.size(applicationStack.width, applicationStack.height), WlShellSurface.NoneEdge)
        if (applicationStack.depth == 1) {
            applicationStack.push(item)
        } else {
            applicationStack.replace(item)
        }
        appLaunching = ''
    }

    ApplicationLauncher {
        id: launcher

        function show(app) {
            if (current === app) return
            if (app.length > 0) {
                if (root.appLaunching.length > 0)
                    return
                if (app2item[app]) {
                    if (applicationStack.depth == 1) {
                        applicationStack.push(app2item[app])
                    } else {
                        applicationStack.replace(app2item[app])
                    }
                    current = app
                } else {
                    root.appLaunching = app
                    if (launch(app) < 0) {
                        root.appLaunching = ''
                    }
                }
            } else if (applicationStack.depth > 1) {
                applicationStack.pop()
                current = ''
            }
        }
    }

    Component {
        id: home
        Home {
            width: parent.width
            height: parent.height
        }
    }

    Transition {
        id: inTransition
        NumberAnimation {
            properties: "opacity, scale"
            from: 0
            to:1
            duration: 250
        }
    }
    Transition {
        id: outTransition
        NumberAnimation {
            properties: "opacity"
            from: 1
            to:0
            duration: 250
        }
        NumberAnimation {
            properties: "scale"
            from: 1
            to:5
            duration: 250
        }
    }

    Image {
        id: container
        anchors.centerIn: parent
        width: 1080
        height: 1920
        rotation: 270
        source: './images/AGL_HMI_Background_NoCar-01.png'

        ColumnLayout {
            anchors.fill: parent
            spacing: 0
            TopArea {
                id: topArea
                Layout.fillWidth: true
                Layout.preferredHeight: 218
            }

            Item {
                id: applicationArea
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 1920 - 218 - 215
                clip: true
                StackView {
                    id: applicationStack
                    anchors.fill: parent
                    initialItem: home

                    pushEnter: inTransition
                    pushExit: outTransition
                    replaceEnter: inTransition
                    replaceExit: outTransition
                    popEnter: inTransition
                    popExit: outTransition
                }
            }

            MediaArea {
                id: mediaArea
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 215
                MouseArea {
                    z: 100
                    anchors.fill: parent
                    onClicked: {
                        notificationLayer.shown = true
                    }
                }
            }
        }
    }

    MouseArea {
        id: notificationLayer
        property bool shown: false
        anchors.fill: container
        scale: container.scale
        visible: opacity > 0
        opacity: 0
        rotation: 270
        z: 100

        onClicked: shown = false

        Rectangle {
            anchors.fill: parent
            color: 'black'
            opacity: 0.75
        }

        states: [
            State {
                name: "notify"
                when: notificationLayer.shown
                PropertyChanges {
                    target: notificationLayer
                    opacity: 1.0
                }
                PropertyChanges {
                    target: timer
                    running: true
                }
            }
        ]

        transitions: [
            Transition {
                NumberAnimation {
                    properties: 'opacity'
                    duration: 250
                    easing.type: Easing.OutExpo
                }
            }
        ]

        Column {
            anchors.centerIn: parent
            spacing: 20
            Rectangle {
                width: 600
                height: 250
                radius: 20
                color: 'white'

                Label {
                    anchors.centerIn: parent
                    text: "Message 1"
                    font.pixelSize: 96
                    color: 'blue'
                    antialiasing: true
                }
            }
            Rectangle {
                width: 600
                height: 250
                radius: 20
                color: 'white'

                Label {
                    anchors.centerIn: parent
                    text: "Restart"
                    font.pixelSize: 96
                    color: 'red'
                    antialiasing: true
                    MouseArea {
                        anchors.fill: parent
                        onClicked: Qt.quit()
                    }
                }
            }
            Timer {
                id: timer
                interval: 3000
                onTriggered: notificationLayer.shown = false
            }
        }
    }
}

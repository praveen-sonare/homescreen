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
import HomeScreen 1.0

Window {
    visible: true
    flags: Qt.FramelessWindowHint
    width: container.width * container.scale
    height: container.height * container.scale
    title: 'HomeScreen'
    color: "#00000000"

    Image {
        id: container
        anchors.centerIn: parent
        width: 1920
        height: 720
        scale: 1.0
        source: './images/menubar_background.png'

        ColumnLayout {
            id: menuBar
            anchors.fill: parent
            spacing: 0
            TopArea {
                id: topArea
                anchors.horizontalCenter: parent.horizontalCenter
                Layout.preferredHeight: 80
                x: 640
            }

            Item {
                id: applicationArea
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 510

                visible: true
                MouseArea {
                    enabled: true
                }
            }

            ShortcutArea {
                id: shortcutArea
                anchors.horizontalCenter: parent.horizontalCenter
                Layout.fillHeight: true
                Layout.preferredHeight: 130
            }
        }
        states: [
            State {
                name: "normal"
                PropertyChanges {
                    target: container
                    y: 0
                }
                PropertyChanges {
                    target: topArea
                    y: 0
                }
                PropertyChanges {
                    target: applicationArea
                    y: 80
                }
                PropertyChanges {
                    target: shortcutArea
                    y: 590
                }
            },
            State {
                name: "fullscreen"
                PropertyChanges {
                    target: container
                    y: -720
                }
                PropertyChanges {
                    target: topArea
                    y: -80
                }
                PropertyChanges {
                    target: applicationArea
                    y: -510
                }
                PropertyChanges {
                    target: shortcutArea
                    y: 720
                }
            }
        ]
        transitions: Transition {
            NumberAnimation {
                target: topArea
                property: "y"
                easing.type: "OutQuad"
                duration: 250
            }
            NumberAnimation {
                target: applicationArea
                property: "y"
                easing.type: "OutQuad"
                duration: 250
            }
            NumberAnimation {
                target: shortcutArea
                property: "y"
                easing.type: "OutQuad"
                duration: 250
            }
        }
    }





    Item {
        id: switchBtn
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.top: parent.top
        anchors.topMargin: 5
        width: 55
        height: 55
        z: 1

        MouseArea {
            anchors.fill: parent
            property string btnState: 'normal'
            Image {
                id: image
                anchors.fill: parent
                source: './images/normal.png'
            }
            onClicked: {
                if (btnState === 'normal') {
                    image.source = './images/fullscreen.png'
                    btnState = 'fullscreen'
                    container.state = 'fullscreen'
                    container.opacity = 0.0
                    touchArea.switchArea(1)

                } else {
                    image.source = './images/normal.png'
                    btnState = 'normal'
                    container.state = 'normal'
                    container.opacity = 1.0
                    touchArea.switchArea(0)
                }
            }
        }
    }
}

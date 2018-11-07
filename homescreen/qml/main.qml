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
        width: 1080
        height: 1920
        scale: 1.0
        source: './images/AGL_HMI_Blue_Background_NoCar-01.png'

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

                visible: true
            }

            MediaArea {
                id: mediaArea
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 215
            }
        }

        states: [
            State {
                name: "normal"
                PropertyChanges {
                    target: topArea
                    y: 0
                }
                PropertyChanges {
                    target: applicationArea
                    y: 218
                }
                PropertyChanges {
                    target: mediaArea
                    y: 1705
                }
            },
            State {
                name: "fullscreen"
                PropertyChanges {
                    target: topArea
                    y: -220
                }
                PropertyChanges {
                    target: applicationArea
                    y: -1490
                }
                PropertyChanges {
                    target: mediaArea
                    y: 2135
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
                target: mediaArea
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
        anchors.topMargin: 25
        width: 35
        height: 35
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
                var appName = homescreenHandler.getCurrentApplication()
                if (btnState === 'normal') {
                    image.source = './images/fullscreen.png'
                    btnState = 'fullscreen'
                    container.state = 'fullscreen'
                    touchArea.switchArea(1)
                    homescreenHandler.tapShortcut(appName, true)
                    if (appName === 'navigation' || appName === 'browser') {
                        container.opacity = 0.0
                    }
                } else {
                    image.source = './images/normal.png'
                    btnState = 'normal'
                    container.state = 'normal'
                    touchArea.switchArea(0)
                    homescreenHandler.tapShortcut(appName, false)
                    if (appName === 'navigation' || appName === 'browser') {
                        container.opacity = 1.0
                    }
                }
            }
        }
    }
}

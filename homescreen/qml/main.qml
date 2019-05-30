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
        id: fullscreen_back
        anchors.centerIn: parent
        width: 1920
        height: 1080
        source: './images/menubar_fullscreen_background.png'
    }

    Image {
        id: container
        anchors.centerIn: parent
        width: 1920
        height: 1080
        scale: 1.0
        source: './images/menubar_background.png'

        ColumnLayout {
            id: menuBar
            width: 1920
            height: 720
//            y:180
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
                    y: 180
                }
                PropertyChanges {
                    target: topArea
                    y: 180
                }
                PropertyChanges {
                    target: applicationArea
                    y: 260
                }
                PropertyChanges {
                    target: shortcutArea
                    y: 770
                }
            },
            State {
                name: "fullscreen"
                PropertyChanges {
                    target: container
                    y: -900
                }
                PropertyChanges {
                    target: topArea
                    y: -260
                }
                PropertyChanges {
                    target: applicationArea
                    y: -590
                }
                PropertyChanges {
                    target: shortcutArea
                    y: 900
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
        width: 61
        height: 61
        anchors.right: parent.right
        anchors.rightMargin: 17
        anchors.top: parent.top
        anchors.topMargin: 182
        z: 1
        Image {
            id: image
            width: 55
            height: 55
            anchors.centerIn: parent
            source: './images/normal.png'
        }

        MouseArea {
            anchors.fill: parent
            property string btnState: 'normal'
            onClicked: {
                if (container.state === 'normal') {
                    turnToFullscreen()
                } else {
                    turnToNormal()
                }
            }
        }
    }

    Item {
        id: splitSwitchBtn
        width: 61
        height: 61
        anchors.right: switchBtn.left
        anchors.top: parent.top
        anchors.topMargin: 182
        z: 1
        property bool enableSplitSwitchBtn: false
        Image {
            id: splitSwitchImage
            width: 55
            height: 55
            anchors.centerIn: parent
            source: './images/split_switch_disable.png'
        }

        MouseArea {
            property bool changed : false
            anchors.fill: parent
            onClicked: {
                if (splitSwitchBtn.enableSplitSwitchBtn) {
                    if(changed) {
                        switchSplitArea(0)
                        changed = false
                    }
                    else {
                        switchSplitArea(1)
                        changed = true
                    }
                }
            }
        }
    }

    Item {
        id: rebootBtn
        width: 70
        height: 70
        anchors.left: parent.left
        anchors.top: parent.top
        z: 1
        MouseArea {
            anchors.fill: parent
            onClicked: {
                homescreenHandler.reboot();
            }
        }
    }


    function turnToFullscreen() {
        image.source = './images/fullscreen.png'
        container.state = 'fullscreen'
        container.opacity = 0.0
        touchArea.switchArea(1)
    }

    function turnToNormal() {
        image.source = './images/normal.png'
        container.state = 'normal'
        container.opacity = 1.0
        touchArea.switchArea(0)
    }

    function enableSplitSwitchBtn() {
        splitSwitchImage.source = './images/split_switch.png'
        splitSwitchBtn.enableSplitSwitchBtn = true
    }

    function disableSplitSwitchBtn() {
        splitSwitchImage.source = './images/split_switch_disable.png'
        splitSwitchBtn.enableSplitSwitchBtn = false;
    }

    function switchSplitArea(val) {
        homescreenHandler.changeLayout(val);
    }
}

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

import QtQuick 2.2
import QtQuick.Layouts 1.1

Item {
    id: root
    width: 700
    height: 110

    Timer {
        id:informationTimer
        interval: 3000
        running: false
        repeat: true
        onTriggered: {
            bottomInformation.visible = false
        }
    }


    ListModel {
        id: applicationModel
        ListElement {
            name: 'launcher'
            application: 'launcher@0.1'
        }
        ListElement {
            name: 'MediaPlayer'
            application: 'mediaplayer@0.1'
        }
        ListElement {
            name: 'navigation'
            application: 'navigation@0.1'
        }
        ListElement {
            name: 'Phone'
            application: 'phone@0.1'
        }
        ListElement {
            name: 'settings'
            application: 'settings@0.1'
        }
    }

    property int pid: -1

    RowLayout {
        anchors.fill: parent
        spacing: 75
        Repeater {
            model: applicationModel
            delegate: ShortcutIcon {
//                Layout.fillWidth: true
//                Layout.fillHeight: true
                width: 60
                height: 60
                name: model.name
                active: model.name === launcher.current
                onClicked: {
//                    if(model.application === 'navigation@0.1') {
//                        pid = launcher.launch('browser@5.0')
//                    } else {
//                        pid = launcher.launch(model.application.toLowerCase())
//                    }

//                    if (1 < pid) {
                        applicationArea.visible = true
//                    }
//                    else {
//                        console.warn(model.application)
//                        console.warn("app cannot be launched!")
//                    }
                    if(model.name === 'Navigation') {
                        homescreenHandler.tapShortcut('browser')
                    } else {
                        homescreenHandler.tapShortcut(model.name)
                    }
                }
            }
        }
    }
    Rectangle {
        id: bottomInformation
        width: parent.width
        height: parent.height-20
        anchors.bottom: parent.bottom
        color: "gray"
        z: 1
        opacity: 0.8
        visible: false

        Text {
            id: informationText
            anchors.centerIn: parent
            font.pixelSize: 25
            font.letterSpacing: 5
            horizontalAlignment: Text.AlignHCenter
            color: "white"
            text: ""
        }
    }

    Connections {
        target: homescreenHandler
        onInformation: {
            informationText.text = text
            bottomInformation.visible = true
            informationTimer.restart()
        }
    }
}

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
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0

MouseArea {
    id: root
    property string name: 'Home'
    property bool active: false
    Item {
        id: icon
        property real desaturation: 0
        anchors.fill: parent
        Image {
            id: inactiveIcon
            anchors.fill: parent
            source: './images/Shortcut/%1.svg'.arg(root.name.toLowerCase())
            fillMode: Image.PreserveAspectFit
        }
        Image {
            id: activeIcon
            anchors.fill: parent
            source: './images/Shortcut/%1_active.svg'.arg(root.name.toLowerCase())
            fillMode: Image.PreserveAspectFit
            opacity: 0.0
        }
        layer.enabled: true
        layer.effect: Desaturate {
            id: desaturate
            desaturation: icon.desaturation
            cached: true
        }
    }
    Label {
        id: name
        width: root.width - 10
        font.pixelSize: 15
        font.letterSpacing: 5
        // wrapMode: Text.WordWrap
        anchors.centerIn: icon
        anchors.verticalCenterOffset: icon.height * 0.2
        //anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignHCenter
        color: "white"
        text: qsTr(model.name.toUpperCase())
    }
    states: [
        State {
            when: launcher.launching
            PropertyChanges {
                target: root
                enabled: false
            }
            PropertyChanges {
                target: icon
                desaturation: 1.0
            }
        },
        State {
            when: root.active
            PropertyChanges {
                target: inactiveIcon
                opacity: 0.0
            }
            PropertyChanges {
                target: activeIcon
                opacity: 1.0
            }
        }
    ]

    transitions: [
        Transition {
            NumberAnimation {
                properties: 'opacity'
                duration: 500
                easing.type: Easing.OutExpo
            }
            NumberAnimation {
                properties: 'desaturation'
                duration: 250
            }
        }
    ]
}

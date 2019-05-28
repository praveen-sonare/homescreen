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
    width: 195
    height: 216.8
    property string name: 'Home'
    property string icon:'./images/Shortcut/'
    property bool active: false
    property bool isBlank: false

    Item {
        id: icon
        property real desaturation: 0
        anchors.fill: parent
        Image {
            id: inactiveIcon
            anchors.fill: parent
            source: './images/Shortcut/blank.svg'
//            source: isBlank ? './images/Shortcut/blank.svg' : root.icon
            property string initial: root.name.substring(0,1).toUpperCase()
            Label {
                style: Text.Outline
                styleColor: '#00FFFF'
                color: 'transparent'
                font.pixelSize: 75
                anchors.centerIn: parent
                text: inactiveIcon.initial
                visible: root.isBlank
            }
        }
        Image {
            id: activeIcon
            anchors.fill: parent
            source: './images/Shortcut/blank_active.svg'
//            source: isBlank ? './images/Shortcut/blank_active.svg' : root.icon
            property string initial: root.name.substring(0,1).toUpperCase()
            Label {
                style: Text.Outline
                styleColor: '#00FFFF'
                color: 'transparent'
                font.pixelSize: 75
                anchors.centerIn: parent
                text: activeIcon.initial
                visible: root.isBlank
            }
            opacity: 0.0
        }
        Image {
            id: sourceIcon
            anchors.fill: parent
            source: isBlank ? null : root.icon
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
        y: 160
        width: root.width - 10
        font.pixelSize: 15
        font.letterSpacing: 5
        // wrapMode: Text.WordWrap
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignHCenter
        color: "white"
        text: qsTr((root.name === "launcher" ? "home" : root.name).toUpperCase())
    }

    states: [
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
}

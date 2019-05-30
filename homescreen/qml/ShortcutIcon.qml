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
    width: 70
    height: 70
    property string name: 'Home'
    property bool active: false
    Item {
        id: icon
        property real desaturation: 0
        anchors.fill: parent
        Image {
            id: inactiveIcon
            anchors.fill: parent
            source: './images/Shortcut/%1.png'.arg(root.name.toLowerCase())
        }
        Image {
            id: activeIcon
            anchors.fill: parent
            source: './images/Shortcut/%1_active.png'.arg(root.name.toLowerCase())
            opacity: 0.0
        }
        layer.enabled: true
        layer.effect: Desaturate {
            id: desaturate
            desaturation: icon.desaturation
            cached: true
        }
    }

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

    onPressed: {
        activeIcon.opacity = 1.0
        inactiveIcon.opacity = 0.0
    }

    onReleased: {
        activeIcon.opacity = 0.0
        inactiveIcon.opacity = 1.0
    }
}

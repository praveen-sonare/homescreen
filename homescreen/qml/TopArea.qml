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
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.0

Image {
    anchors.fill: parent
    source: './images/TopSection_NoText_NoIcons-01.svg'
    //fillMode: Image.PreserveAspectCrop
    fillMode: Image.Stretch

    RowLayout {
        anchors.fill: parent
        spacing: 0
        ShortcutArea {
            id: shortcutArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 775
        }
        StatusArea {
            id: statusArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 291
        }
    }
/*
    Timer {
        id: launching
        interval: 500
        running: launcher.launching
    }

    ProgressBar {
        id: progressBar
        anchors.verticalCenter: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        indeterminate: visible
        visible: launcher.launching && !launching.running
    }
*/
}

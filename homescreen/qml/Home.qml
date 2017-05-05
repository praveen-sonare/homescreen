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
import Home 1.0

Item {
    id: root

    Image {
        anchors.fill: parent
        anchors.topMargin: -218
        anchors.bottomMargin: -215
        source: './images/AGL_HMI_Background_Car-01.png'
    }

    property int pid: -1

    GridView {
        anchors.centerIn: parent
        width: cellWidth * 3
        height: parent.height
        cellWidth: 320
        cellHeight: 320
        interactive: true
        clip: true

        model: ApplicationModel {}

        delegate: MouseArea {
            width: 320
            height: 320

            Item {
                width: parent.width
                height: parent.height
                anchors.fill: parent

                // These images contain the item text at the bottom,
                // is it possible to crop the image in QML?
                Image {
                    id: img
                    clip: true
                    width: parent.width
                    height: parent.height
                    fillMode: Image.PreserveAspectCrop
                    source: model.icon.substr(0, 5) === "file:"
                        ? model.icon
                        : './images/HMI_AppLauncher_%1_%2-01.png'.arg(model.icon).arg(pressed ? 'Active' : 'Inactive')
                }

                // Show this rect and the text below if the image could not be loaded
                Rectangle {
                    anchors {
                        fill: parent
                        margins: 50
                    }
                    border {
                        color: "#64fdcb"
                        width: 3
                    }
                    color: pressed ? "#11bcb9" : "#202020"
                    radius: parent.width / 2 // circle'd rectangle...
                    visible: img.status == Image.Error
                }

                Text {
                    property int font_size: 26

                    font.family: roboto
                    font.capitalization: Font.AllUppercase
                    font.bold: false
                    font.pixelSize: font_size
                    text: model.name
                    color: "#ffffff"
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        top: img.top
                        topMargin: parent.height / 2 - font_size / 2
                    }
                    visible: img.status == Image.Error
                }
            }

            onClicked: {
                console.log("app is ", model.id)
                pid = launcher.launch(model.id)
                if (1 < pid) {
                    layoutHandler.makeMeVisible(pid)

                    applicationArea.visible = true
                    appLauncherAreaLauncher.visible = false
                    layoutHandler.showAppLayer(pid)
                }
                else {
                    console.warn("app cannot be launched!")
                }
            }
        }
    }
}

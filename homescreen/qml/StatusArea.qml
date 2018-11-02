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
import HomeScreen 1.0

Item {
    id: root
    width: 700
    height: 80

    property date now: new Date
    Timer {
        interval: 100; running: true; repeat: true;
        onTriggered: root.now = new Date
    }

    Timer {
        id:notificationTimer
        interval: 3000
        running: false
        repeat: true
        onTriggered: notificationItem.visible = false
    }

    Connections {
        target: weather

        onConditionChanged: {
            var icon = ''

            if (condition.indexOf("clouds") !== -1) {
                icon = "WeatherIcons_Cloudy-01.png"
            } else if (condition.indexOf("thunderstorm") !== -1) {
                icon = "WeatherIcons_Thunderstorm-01.png"
            } else if (condition.indexOf("snow") !== -1) {
                icon = "WeatherIcons_Snow-01.png"
            } else if (condition.indexOf("rain") !== -1) {
                icon = "WeatherIcons_Rain-01.png"
            }

            condition_item.source = icon ? './images/Weather/' + icon : ''
        }

        onTemperatureChanged: {
            temperature_item.text = temperature.split(".")[0] + '°F'
        }
    }

        RowLayout {
            anchors.fill: parent
            spacing: 0
            RowLayout {
                id: icons
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 120
                spacing: -10

                Image {
                    id: bt_icon
                    Layout.preferredWidth: 50
                    Layout.preferredHeight: 50
                    source: connStatus ? './images/Status/HMI_Status_Bluetooth_On-01.png' : './images/Status/HMI_Status_Bluetooth_Inactive-01.png'
                    fillMode: Image.PreserveAspectFit
                    property string deviceName: "none"
                    property bool connStatus: false
                    Connections {
                        target: bluetooth
                        onConnectionEvent: {
                            console.log("onConnectionEvent", data.Status)
                            if (data.Status === "connected") {
                                bt_icon.connStatus = true
                            } else if (data.Status === "disconnected") {
                                bt_icon.connStatus = false
                            }
                        }
                        onDeviceUpdateEvent: {
                            console.log("onConnectionEvent", data.Paired)
                            if (data.Paired === "True" && data.Connected === "True") {
                                bt_icon.deviceName = data.name
                                bt_icon.connStatus = true
                            } else {
                                if(bt_icon.deviceName === data.Name)
                                {
                                    bt_icon.connStatus = false
                                }
                            }
                        }
                    }
                }

                Repeater {
                    model: StatusBarModel { objectName: "statusBar" }
                    delegate: Image {
                        Layout.preferredWidth: 50
                        Layout.preferredHeight: 50
                        source: model.modelData
                        fillMode: Image.PreserveAspectFit
                    }
                }
            }
            Item {
                anchors.left: icons.right
                Layout.fillHeight: true
                width: 440
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 17
                    spacing: 0
                    Text {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        text: Qt.formatDate(now, 'dddd').toUpperCase()
                        font.family: 'Roboto'
                        font.pixelSize: 13
                        color: 'white'
                        horizontalAlignment:  Text.AlignHCenter
                        verticalAlignment:  Text.AlignVCenter
                    }
                    Text {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        text: Qt.formatTime(now, 'h:mm ap').toUpperCase()
                        font.family: 'Roboto'
                        font.pixelSize: 38
                        color: 'white'
                        horizontalAlignment:  Text.AlignHCenter
                        verticalAlignment:  Text.AlignVCenter
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 20

                Image {
                    id: condition_item
                    source: './images/Weather/WeatherIcons_Rain-01.png'
                }
                Text {
                    id: temperature_item
                    text: '64°F'
                    color: 'white'
                    font.family: 'Helvetica'
                    font.pixelSize: 32
                }
            }
        }

        Item {
            id: notificationItem
            x: 0
            y: 0
            z: 1
            width: parent.width
            height: 100
            opacity: 0.8
            visible: false

            Rectangle {
                width: parent.width
                height: parent.height
                anchors.fill: parent
                color: "gray"
                Image {
                    id: notificationIcon
                    width: 70
                    height: 70
                    anchors.left: parent.left
                    anchors.leftMargin: 20
                    anchors.verticalCenter: parent.verticalCenter
                    source: ""
                }

                Text {
                    id: notificationtext
                    font.pixelSize: 25
                    anchors.left: notificationIcon.right
                    anchors.leftMargin: 5
                    anchors.verticalCenter: parent.verticalCenter
                    color: "white"
                    text: qsTr("")
                }
            }
        }

        Connections {
            target: homescreenHandler
            onNotification: {
                notificationIcon.source = './images/Shortcut/%1.svg'.arg(id)
                notificationtext.text = text
                notificationItem.visible = true
                notificationTimer.restart()
            }
        }

}

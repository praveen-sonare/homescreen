/*
 * Copyright (C) 2016 The Qt Company Ltd.
 * Copyright (C) 2016, 2017 Mentor Graphics Development (Deutschland) GmbH
 * Copyright (c) 2017, 2018 TOYOTA MOTOR CORPORATION
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
    width: 295
    height: 218

    property date now: new Date
    Timer {
        interval: 100; running: true; repeat: true;
        onTriggered: root.now = new Date
    }

    Connections {
        target: weather

        onConditionChanged: {
            var icon = ''

            if (condition.indexOf("clouds") != -1) {
                icon = "WeatherIcons_Cloudy-01.png"
            } else if (condition.indexOf("thunderstorm") != -1) {
                icon = "WeatherIcons_Thunderstorm-01.png"
            } else if (condition.indexOf("snow") != -1) {
                icon = "WeatherIcons_Snow-01.png"
            } else if (condition.indexOf("rain") != -1) {
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
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 295 - 76
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 40
                spacing: 0
                Text {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: Qt.formatDate(now, 'dddd').toUpperCase()
                    font.family: 'Roboto'
                    font.pixelSize: 13
                    color: 'white'
                    verticalAlignment:  Text.AlignVCenter
//                    Rectangle {
//                        anchors.fill: parent
//                        anchors.margins: 5
//                        color: 'red'
//                        border.color: 'blue'
//                        border.width: 1
//                        z: -1
//                    }
                }
                Text {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: Qt.formatTime(now, 'h:mm ap').toUpperCase()
                    font.family: 'Roboto'
                    font.pixelSize: 40
                    color: 'white'
                    horizontalAlignment:  Text.AlignHCenter
                    verticalAlignment:  Text.AlignVCenter
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
        }
        ColumnLayout {
            id: icons
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 76
            spacing: -10

            Image {
                id: bt_icon
                Layout.preferredWidth: 77
                Layout.preferredHeight: 73
                source: connStatus ? './images/Status/HMI_Status_Bluetooth_On-01.png' : './images/Status/HMI_Status_Bluetooth_Inactive-01.png'
                fillMode: Image.PreserveAspectFit
                property string deviceName: "none"
                property bool connStatus: false
                Connections {
                    target: bluetooth

                    onPowerChanged: {
                            bt_icon.connStatus = state
                    }
                }
            }
            Repeater {
                model: StatusBarModel { objectName: "statusBar" }
                delegate: Image {
                    Layout.preferredWidth: 77
                    Layout.preferredHeight: 73
                    source: model.modelData
                    fillMode: Image.PreserveAspectFit
                }
            }
        }
    }
}

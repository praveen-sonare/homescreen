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

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 217
            spacing: 0

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 130
                spacing: 0

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredHeight: 70

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: parent.width * 0.2

                        text: Qt.formatDate(now, 'dddd').toUpperCase()
                        font.family: 'Roboto'
                        font.pixelSize: 13
                        color: 'white'
                        verticalAlignment:  Text.AlignVCenter
                        fontSizeMode: Text.Fit
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredHeight: 60

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: parent.height * 0.05

                        text: Qt.formatTime(now, 'h:mm ap').toUpperCase()
                        font.family: 'Roboto'
                        font.pixelSize: 40
                        color: 'white'
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        fontSizeMode: Text.Fit
                    }
                }
            }
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 82

                RowLayout {
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.topMargin: parent.height * 0.1

                    Image {
                        id: condition_item
                        source: './images/Weather/WeatherIcons_Rain-01.png'
                        fillMode: Image.PreserveAspectFit
                    }
                    Text {
                        id: temperature_item
                        text: '64°F'
                        color: 'white'
                        font.family: 'Helvetica'
                        font.pixelSize: 32
                        fontSizeMode: Text.Fit
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
        ColumnLayout {
            id: icons
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 74
            spacing: -10

            Image {
                id: bt_icon
                Layout.preferredHeight: 70
                Layout.fillWidth: true
                Layout.fillHeight: true
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
                    Layout.preferredHeight: 70
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    source: model.modelData
                    fillMode: Image.PreserveAspectFit
                }
            }
        }
    }
}

/*
 * Copyright (C) 2016 The Qt Company Ltd.
 * Copyright (C) 2016, 2017 Mentor Graphics Development (Deutschland) GmbH
 * Copyright (c) 2017, 2018, 2019 TOYOTA MOTOR CORPORATION
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
import QtQuick.Window 2.2

Item {
    id: root

    ListModel {
        id: applicationModel
        ListElement {
            appid: "launcher"
            name: 'launcher'
            application: 'launcher'
        }
        ListElement {
            appid: "mediaplayer"
            name: 'MediaPlayer'
            application: 'mediaplayer'
        }
        ListElement {
            appid: "hvac"
            name: 'HVAC'
            application: 'hvac'
        }
        ListElement {
            appid: "navigation"
            name: 'Navigation'
            application: 'navigation'
        }
    }

	property int pid: -1

	property string current_appid: ''
	property variant current_window: {}

	property variant applications: {'' : -1}

	function find_app(app, apps) {
		for (var x in apps) {

			if (apps[x] == -1)
				continue

			if (x === app)
				return true
		}
		return false
	}

	function set_current_window_app(appid, window) {
		current_appid = appid
		current_window = window
	}


    Timer {
	id: timer
	interval: 500
	running: false
	repeat: false
	onTriggered: {
		if (current_appid != '' && current_window != undefined) {
			console.log("Timer expired, switching to " + current_appid)
			shell.activate_app(current_window, current_appid)
		}
	}
    }


    RowLayout {
        anchors.fill: parent
        spacing: 0
        Repeater {
            model: applicationModel
            delegate: ShortcutIcon {
                Layout.fillWidth: true
                Layout.fillHeight: true
                name: model.name
                active: model.name === launcher.current

		onClicked: {
			// if timer still running ignore
			if (timer.running) {
				console.log("Timer still running")
				return
			}

			// find the app before trying to start
			if (find_app(model.application, applications)) {
				console.log("application " + model.appid + "already started. Just switching")
				shell.activate_app(Window.window, model.appid)
				return
			}

			pid = launcher.launch(model.application)
			if (pid > 0) {
				set_current_window_app(model.appid, Window.window)
				applications[model.application] = pid
				timer.running = true
			}
		}
            }
        }
    }
}

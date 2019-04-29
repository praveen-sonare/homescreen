import QtQuick 2.0
import SpeechChrome 1.0

Item {
    id: root

    clip: true

    property bool agentPresent: speechChromeController.agentPresent

    visible: agentPresent

    Image {
        id: chromeBarImage

        anchors.top: parent.top
        source: "./images/SpeechChrome/bar.png"

        Behavior on x {
            NumberAnimation { duration: 250 }
        }
        Behavior on opacity {
            NumberAnimation { duration: 250 }
        }
    }

    Image {
        id: pushToTalk

        height: parent.height * 0.80
        width: height

        anchors.left: parent.left
        anchors.leftMargin: parent.width / 128
        anchors.verticalCenter: parent.verticalCenter
        source: "./images/SpeechChrome/push_to_talk.svg"

        MouseArea {
            anchors.fill: parent
            onPressed: speechChromeController.pushToTalk()
        }

        Behavior on opacity {
            NumberAnimation { duration: 250 }
        }
    }

    states: [
        State {
            name: "Idle"
            when: speechChromeController.chromeState == SpeechChromeController.Idle
            PropertyChanges {
                target: chromeBarImage
                opacity: 0.0
                x: 0
            }
            PropertyChanges {
                target: pushToTalk
                opacity: 1.0
                enabled: true
            }
        },
        State {
            name: "Listening"
            when: speechChromeController.chromeState == SpeechChromeController.Listening
            PropertyChanges {
                target: chromeBarImage
                opacity: 1.0
                x: 0
            }
            PropertyChanges {
                target: pushToTalk
                opacity: 0.0
                enabled: false
            }
        },
        State {
            name: "Thinking"
            when: speechChromeController.chromeState == SpeechChromeController.Thinking
            PropertyChanges {
                target: chromeBarImage
                opacity: 1.0
                x: root.width - chromeBarImage.width
            }
            PropertyChanges {
                target: pushToTalk
                opacity: 0.0
                enabled: false
            }
        },
        State {
            name: "Speaking"
            when: speechChromeController.chromeState == SpeechChromeController.Speaking
            PropertyChanges {
                target: chromeBarImage
                opacity: 1.0
                x: (root.width - chromeBarImage.width) * 0.5
            }
            PropertyChanges {
                target: pushToTalk
                opacity: 0.0
                enabled: false
            }
        },
        State {
            name: "MicrophoneOff"
            when: speechChromeController.chromeState == SpeechChromeController.MicrophoneOff
            PropertyChanges {
                target: chromeBarImage
                opacity: 0.0
                x: 0
            }
            PropertyChanges {
                target: pushToTalk
                opacity: 1.0
                enabled: true
            }
        }
    ]
}

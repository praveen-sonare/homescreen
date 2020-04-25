import QtQuick 2.13
import QtQuick.Window 2.13

Window {
    id: bottompanel
    width: Screen.width
    height: Screen.height * (215.0 / 1920.0)
    flags: Qt.FramelessWindowHint
    visible: true
    //color: "#aaaa0000"
    MediaArea {
    }

    Timer {
        id:informationTimer
        interval: 3000
        running: false
        repeat: true
        onTriggered: {
            bottomInformation.visible = false
        }
    }

    Item {
        id: bottomInformation
        width: parent.width
        height: 215
        anchors.bottom: parent.bottom
        visible: false
        Text {
            id: bottomText
            anchors.centerIn: parent
            font.pixelSize: 25
            font.letterSpacing: 5
            horizontalAlignment: Text.AlignHCenter
            color: "white"
            text: ""
            z:1
        }
    }

    Connections {
        target: homescreenHandler
        onShowInformation: {
            bottomText.text = info
            bottomInformation.visible = true
            informationTimer.restart()
        }
    }
}

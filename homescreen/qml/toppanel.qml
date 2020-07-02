import QtQuick 2.13
import QtQuick.Window 2.13

Window {
    id: toppanel
    width: Screen.width
    height: Screen.height * (218.0 / 1920.0)
    flags: Qt.FramelessWindowHint
    visible: true
    //color: "#aaaa0000"

    TopArea {
    }

    Timer {
        id:notificationTimer
        interval: 3000
        running: false
        repeat: true
        onTriggered: notificationItem.visible = false
    }

    Item {
        id: notificationItem
        x: 0
        y: 0
        z: 1
        width: 1280
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
        onShowNotification: {
            notificationIcon.source = icon_path
            notificationtext.text = text
            notificationItem.visible = true
            notificationTimer.restart()
        }
    }
}

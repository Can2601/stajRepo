import QtQuick

Item {
    id: welcomePage

    property string userId: ""
    signal logoutRequested()

    Rectangle {
        anchors.fill: parent
        color: "#F5F5F5"
    }

    Column {
        anchors.centerIn: parent
        spacing: 20

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "✓ Login Successful"
            font.pixelSize: 28
            font.bold: true
            color: "#2E7D32"
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Welcome, " + welcomePage.userId
            font.pixelSize: 14
            color: "#555555"
        }

        // Sign Out button
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 120
            height: 36
            radius: 6
            color: outMouse.containsPress ? "#c0392b"
                 : outMouse.containsMouse ? "#e74c3c" : "#e74c3c"

            Behavior on color { ColorAnimation { duration: 100 } }

            Text {
                anchors.centerIn: parent
                text: "Sign Out"
                color: "#ffffff"
                font.pixelSize: 13
                font.bold: true
            }

            MouseArea {
                id: outMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: welcomePage.logoutRequested()
            }
        }
    }
}

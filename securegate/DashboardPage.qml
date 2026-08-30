import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic


Page {
    id: dashboardPage

    Rectangle {
        anchors.centerIn: parent
        color: "pink"
        anchors.fill: parent

        Column {
            anchors.centerIn: parent
            spacing: 10

            Text {
                text: "Login successful!"
                font.bold: true
                font.pixelSize: 20
            }
        }
    }
}
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic


ApplicationWindow {
    width: 380
    height: 370
    minimumWidth: 200
    minimumHeight: 250
    visible: true
    title: qsTr("Secure Gate")

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
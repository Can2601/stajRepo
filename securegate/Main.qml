import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

ApplicationWindow {
    width: 470
    height: 460
    minimumWidth: 460
    minimumHeight: 460
    visible: true
    title: qsTr("Secure Gate")

    StackView {
        id: stackView
        anchors.fill: parent

        initialItem: "LoginPage.qml"
    }
}
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

Item {
    id: loginPage
    anchors.fill: parent

    signal loginSucceeded(string userId)
    signal loginFailed(string reason)

    property string _errorText: ""

    // Background
    Rectangle {
        anchors.fill: parent
        color: "#F5F5F5"
    }

    // Card
    Rectangle {
        width: 360
        height: cardColumn.implicitHeight + 48
        anchors.centerIn: parent
        color: "#FFFFFF"
        radius: 10
        border.color: "#DDDDDD"
        border.width: 1

        ColumnLayout {
            id: cardColumn
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                margins: 32
            }
            spacing: 12

            // Title
            Text {
                Layout.fillWidth: true
                Layout.topMargin: 16
                text: qsTr("Sign In")
                font.pixelSize: 22
                font.weight: Font.Bold
                color: "#1A1A1A"
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                Layout.fillWidth: true
                Layout.bottomMargin: 8
                text: qsTr("Enter your credentials to continue")
                font.pixelSize: 13
                color: "#888888"
                horizontalAlignment: Text.AlignHCenter
            }

            // Error message
            Text {
                Layout.fillWidth: true
                text: loginPage._errorText
                color: "#CC0000"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                visible: loginPage._errorText !== ""
            }

            // User ID label + field
            Text {
                text: qsTr("User ID")
                font.pixelSize: 13
                font.weight: Font.Medium
                color: "#444444"
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 38
                radius: 6
                color: "#FAFAFA"
                border.color: idField.activeFocus ? "#4A90E2" : "#CCCCCC"
                border.width: 1

                Behavior on border.color { ColorAnimation { duration: 120 } }

                TextInput {
                    id: idField
                    anchors { fill: parent; leftMargin: 10; rightMargin: 10 }
                    verticalAlignment: TextInput.AlignVCenter
                    color: "#1A1A1A"
                    font.pixelSize: 14
                    clip: true

                    Keys.onReturnPressed: passwordField.forceActiveFocus()
                    Keys.onEnterPressed:  passwordField.forceActiveFocus()

                    Text {
                        anchors.fill: parent
                        verticalAlignment: Text.AlignVCenter
                        text: qsTr("User ID")
                        color: "#AAAAAA"
                        font: idField.font
                        visible: idField.text === ""
                    }
                }
            }

            // Password label + field
            Text {
                text: qsTr("Password")
                font.pixelSize: 13
                font.weight: Font.Medium
                color: "#444444"
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 38
                radius: 6
                color: "#FAFAFA"
                border.color: passwordField.activeFocus ? "#4A90E2" : "#CCCCCC"
                border.width: 1

                Behavior on border.color { ColorAnimation { duration: 120 } }

                TextInput {
                    id: passwordField
                    anchors { fill: parent; leftMargin: 10; rightMargin: 10 }
                    verticalAlignment: TextInput.AlignVCenter
                    echoMode: TextInput.Password
                    color: "#1A1A1A"
                    font.pixelSize: 14
                    clip: true

                    Keys.onReturnPressed: attemptLogin()
                    Keys.onEnterPressed:  attemptLogin()

                    Text {
                        anchors.fill: parent
                        verticalAlignment: Text.AlignVCenter
                        text: qsTr("Password")
                        color: "#AAAAAA"
                        font: passwordField.font
                        visible: passwordField.text === ""
                    }
                }
            }

            // Sign In button
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.bottomMargin: 8
                implicitHeight: 40
                radius: 6
                color: btnMouse.containsPress ? "#3A7BD5" : (btnMouse.containsMouse ? "#4A8EE8" : "#4A90E2")

                Behavior on color { ColorAnimation { duration: 100 } }

                Text {
                    anchors.centerIn: parent
                    text: qsTr("Sign In")
                    color: "#FFFFFF"
                    font.pixelSize: 14
                    font.weight: Font.SemiBold
                }

                MouseArea {
                    id: btnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: attemptLogin()
                }
            }
        }
    }

    // Auth logic
    function attemptLogin() {
        loginPage._errorText = ""
        var id  = idField.text.trim()
        var pwd = passwordField.text

        if (id === "") {
            loginPage._errorText = qsTr("Please enter your User ID.")
            idField.forceActiveFocus()
            return
        }
        if (pwd === "") {
            loginPage._errorText = qsTr("Please enter your password.")
            passwordField.forceActiveFocus()
            return
        }

        // ── License validation via LicenseAuth (C++ backend) ──────────────
        var ok = LicenseAuth.validateCredentials(id, pwd)
        if (ok)
            loginPage.loginSucceeded(id)
        else
            loginPage._errorText = LicenseAuth.lastError()
        // ──────────────────────────────────────────────────────────────────
    }

    Component.onCompleted: idField.forceActiveFocus()
}

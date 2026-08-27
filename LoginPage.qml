import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

Item {
    id: loginPage

    signal loginSucceeded(string userId)
    signal loginFailed(string reason)

    property string _errorText: ""
    property var    _recentList: []

    function refreshList() {
        _recentList = RecentLogins.all()
    }

    // ── Background ────────────────────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: "#F5F5F5"
    }

    // ── Main row: login card + (optional) saved logins panel ──────────────────
    Row {
        id: mainRow
        anchors.centerIn: parent
        spacing: 20

        // ── Login card ────────────────────────────────────────────────────────
        Rectangle {
            id: loginCard
            width: 360
            height: cardColumn.implicitHeight + 48
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
                    color: btnMouse.containsPress ? "#3A7BD5"
                         : btnMouse.containsMouse ? "#4A8EE8" : "#4A90E2"

                    Behavior on color { ColorAnimation { duration: 100 } }

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Sign In")
                        color: "#FFFFFF"
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
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

        // ── Saved logins panel ────────────────────────────────────────────────
        Rectangle {
            id: savedPanel
            visible: loginPage._recentList.length > 0
            width: visible ? 210 : 0
            height: loginCard.height
            color: "#FFFFFF"
            radius: 10
            border.color: "#DDDDDD"
            border.width: 1
            clip: true

            // Slide-in animation when the panel first becomes visible
            Behavior on width { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

            // ── Fixed header (not scrollable) ────────────────────────────
            Column {
                id: panelHeader
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                    margins: 14
                }
                spacing: 6
                topPadding: 6

                Text {
                    text: "Saved Logins"
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    color: "#888888"
                    font.letterSpacing: 0.5
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#EEEEEE"
                }
            }

            // ── Scrollable list ───────────────────────────────────────
            Flickable {
                anchors {
                    top: panelHeader.bottom
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                    leftMargin: 14
                    rightMargin: 14
                    topMargin: 8
                    bottomMargin: 10
                }
                clip: true
                contentHeight: itemsColumn.implicitHeight

                // Thin scrollbar — only visible when content overflows
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                Column {
                    id: itemsColumn
                    width: parent.width
                    spacing: 4

                    Repeater {
                        model: loginPage._recentList

                        delegate: Rectangle {
                            id: delegateRect
                            width: parent.width
                            implicitHeight: 42
                            radius: 7
                            color: itemMouse.containsMouse ? "#EEF4FF" : "#FAFAFA"
                            border.color: itemMouse.containsMouse ? "#4A90E2" : "#EEEEEE"
                            border.width: 1

                            Behavior on color        { ColorAnimation { duration: 80 } }
                            Behavior on border.color { ColorAnimation { duration: 80 } }

                            // Full-area click → autofill ID
                            MouseArea {
                                id: itemMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    idField.text = modelData
                                    passwordField.forceActiveFocus()
                                }
                            }

                            // Content row (rendered on top of itemMouse)
                            RowLayout {
                                anchors { fill: parent; leftMargin: 10; rightMargin: 8 }
                                spacing: 4

                                // User icon
                                Text {
                                    text: "👤"
                                    font.pixelSize: 12
                                    opacity: 0.55
                                }

                                // ID label
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData
                                    font.pixelSize: 13
                                    color: "#1A1A1A"
                                    elide: Text.ElideRight
                                }

                                // Remove (×) button
                                Rectangle {
                                    id: removeBtn
                                    width: 20
                                    height: 20
                                    radius: 10
                                    z: 1
                                    color: removeMouse.containsMouse ? "#FFEEEE" : "transparent"

                                    Behavior on color { ColorAnimation { duration: 80 } }

                                    Text {
                                        anchors.centerIn: parent
                                        text: "✕"
                                        font.pixelSize: 9
                                        color: removeMouse.containsMouse ? "#CC0000" : "#BBBBBB"
                                    }

                                    MouseArea {
                                        id: removeMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.ArrowCursor
                                        onClicked: {
                                            RecentLogins.remove(modelData)
                                            loginPage.refreshList()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ── Auth logic ────────────────────────────────────────────────────────────
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

        // ── License validation via LicenseAuth (C++ backend) ──────────────────
        var ok = LicenseAuth.validateCredentials(id, pwd)
        if (ok) {
            RecentLogins.add(id)        // persist the ID for future sessions
            loginPage.refreshList()     // update the panel immediately
            loginPage.loginSucceeded(id)
        } else {
            loginPage._errorText = LicenseAuth.lastError()
        }
        // ──────────────────────────────────────────────────────────────────────
    }

    Component.onCompleted: {
        loginPage.refreshList()   // load saved IDs from QSettings on startup
        idField.forceActiveFocus()
    }
}

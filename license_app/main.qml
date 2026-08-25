import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15

Window {
    width: 500
    height: 460
    visible: true
    title: qsTr("Lisans Üretici")
    color: "#f5f5f5"

    property string lastId: ""
    property string lastPassword: ""
    property string lastExpiryDate: ""
    property string lastFilePath: ""
    property string lastError: ""
    property bool generated: false

    // Reusable copy button component
    component CopyButton: Rectangle {
        id: copyBtn
        property string copyText: ""
        property string _label: "Kopyala"

        implicitWidth: 72
        implicitHeight: 26
        radius: 4
        color: copyMouse.containsPress ? "#3A7BD5"
             : copyMouse.containsMouse ? "#4A8EE8" : "#4A90E2"

        Behavior on color { ColorAnimation { duration: 80 } }

        Text {
            anchors.centerIn: parent
            text: copyBtn._label
            color: "#ffffff"
            font.pixelSize: 11
            font.bold: true
        }

        MouseArea {
            id: copyMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                copyBtn._label = "Kopyalandı!"
                clipboardHelper.text = copyBtn.copyText
                clipboardHelper.selectAll()
                clipboardHelper.copy()
                resetTimer.start()
            }
        }

        Timer {
            id: resetTimer
            interval: 1500
            onTriggered: copyBtn._label = "Kopyala"
        }
    }

    // Off-screen invisible TextEdit used as clipboard bridge
    TextEdit {
        id: clipboardHelper
        visible: false
        width: 0; height: 0
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Text {
            text: "Lisans Üretici"
            font.pixelSize: 20
            font.bold: true
            color: "#1a1a1a"
        }

        // Generate button
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 40
            radius: 6
            color: btnMouse.containsPress ? "#3A7BD5"
                 : btnMouse.containsMouse ? "#4A8EE8" : "#4A90E2"

            Behavior on color { ColorAnimation { duration: 100 } }

            Text {
                anchors.centerIn: parent
                text: "Lisans Üret"
                color: "#ffffff"
                font.pixelSize: 14
                font.bold: true
            }

            MouseArea {
                id: btnMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    var result = licenseManager.generateAndSave(365)
                    lastId         = result.id         || ""
                    lastPassword   = result.password   || ""
                    lastExpiryDate = result.expiryDate || ""
                    lastFilePath   = result.filePath   || ""
                    lastError      = result.error      || ""
                    generated      = result.success === true
                }
            }
        }

        // Error card
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: errText.implicitHeight + 24
            visible: !generated && lastError !== ""
            color: "#fff0f0"
            radius: 8
            border.color: "#ffaaaa"
            border.width: 1

            Text {
                id: errText
                anchors { fill: parent; margins: 12 }
                text: "Hata: " + lastError
                color: "#cc0000"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
        }

        // Result card
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: resultColumn.implicitHeight + 24
            visible: generated
            color: "#ffffff"
            radius: 8
            border.color: "#cccccc"
            border.width: 1

            ColumnLayout {
                id: resultColumn
                anchors { fill: parent; margins: 12 }
                spacing: 6

                // ── ID row ──────────────────────────────────────────────
                Text { text: "User ID:"; font.pixelSize: 11; font.bold: true; color: "#555555" }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    TextEdit {
                        id: idField
                        Layout.fillWidth: true
                        text: lastId
                        readOnly: true
                        selectByMouse: true
                        font.pixelSize: 12
                        color: "#1a1a1a"
                        wrapMode: TextEdit.WrapAnywhere
                    }

                    CopyButton { copyText: lastId }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#eeeeee" }

                // ── Password row ─────────────────────────────────────────
                Text { text: "Şifre:"; font.pixelSize: 11; font.bold: true; color: "#555555" }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    TextEdit {
                        id: pwField
                        Layout.fillWidth: true
                        text: lastPassword
                        readOnly: true
                        selectByMouse: true
                        font.pixelSize: 13
                        font.bold: true
                        color: "#1a1a1a"
                    }

                    CopyButton { copyText: lastPassword }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#eeeeee" }

                // ── Expiry row ───────────────────────────────────────────
                Text { text: "Son Kullanma Tarihi:"; font.pixelSize: 11; font.bold: true; color: "#555555" }
                Text { text: lastExpiryDate; font.pixelSize: 13; color: "#1a1a1a" }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#eeeeee" }

                // ── File path row ────────────────────────────────────────
                Text {
                    text: "Kaydedildi: " + lastFilePath
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                    font.pixelSize: 10
                    color: "#888888"
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}

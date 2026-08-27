import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls

Window {
    width: 500
    height: 560
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

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: inputId
                Layout.fillWidth: true
                placeholderText: "Kullanıcı ID Girin"
                leftPadding: 12
                background: Rectangle {
                    radius: 8
                    color: "#ffffff"
                    border.color: parent.activeFocus ? "#4A90E2" : "#dddddd"
                    border.width: parent.activeFocus ? 2 : 1
                }
            }

            Button {
                text: "🎲"
                implicitWidth: 40
                implicitHeight: 40
                font.pixelSize: 18
                padding: 0
                onClicked: {
                    let chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
                    let res = ""
                    let randomLength = Math.floor(Math.random() * 5) + 6
                    for (let i = 0; i < randomLength; i++) {
                        res += chars.charAt(Math.floor(Math.random() * chars.length))
                    }
                    inputId.text = res
                    lastError = ""
                }
                background: Rectangle {
                    radius: 8
                    color: parent.hovered ? "#f0f4f8" : "#ffffff"
                    border.color: parent.hovered ? "#4A90E2" : "#dddddd"
                    border.width: 1
                }
            }
        }

        // --- PASSWORD INPUT AREA ---
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: inputPassword
                Layout.fillWidth: true
                placeholderText: "Lisans Şifresini Girin"
                leftPadding: 12
                background: Rectangle {
                    radius: 8
                    color: "#ffffff"
                    border.color: parent.activeFocus ? "#4A90E2" : "#dddddd"
                    border.width: parent.activeFocus ? 2 : 1
                }
            }

            Button {
                text: "🎲"
                implicitWidth: 40
                implicitHeight: 40
                font.pixelSize: 18
                padding: 0
                onClicked: {
                    let upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    let lower = "abcdefghijklmnopqrstuvwxyz"
                    let num = "0123456789"
                    let special = "!@#$%^&*_"
                    let all = upper + lower + num + special

                    let res = ""
                    res += upper.charAt(Math.floor(Math.random() * upper.length))
                    res += lower.charAt(Math.floor(Math.random() * lower.length))
                    res += special.charAt(Math.floor(Math.random() * special.length))
                    res += num.charAt(Math.floor(Math.random() * num.length))

                    let extraLength = Math.floor(Math.random() * 7) + 6
                    for (let i = 0; i < extraLength; i++) {
                        res += all.charAt(Math.floor(Math.random() * all.length))
                    }

                    let shuffled = res.split('').sort(function(){return 0.5-Math.random()}).join('')

                    inputPassword.text = shuffled
                    lastError = ""
                }
                background: Rectangle {
                    radius: 8
                    color: parent.hovered ? "#f0f4f8" : "#ffffff"
                    border.color: parent.hovered ? "#4A90E2" : "#dddddd"
                    border.width: 1
                }
            }
        }

        // --- DATE INPUT AREA ---
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: inputDate
                Layout.fillWidth: true
                placeholderText: "Bitiş Tarihi (Örn: 25.08.2027)"
                font.pixelSize: 14
                leftPadding: 12
                background: Rectangle {
                    radius: 8
                    color: "#ffffff"
                    border.color: parent.activeFocus ? "#4A90E2" : "#dddddd"
                    border.width: parent.activeFocus ? 2 : 1
                }
            }

            Button {
                text: "📅"
                implicitWidth: 40
                implicitHeight: 40
                font.pixelSize: 18
                padding: 0
                onClicked: calendarPopup.open()
                background: Rectangle {
                    radius: 8
                    color: parent.hovered ? "#f0f4f8" : "#ffffff"
                    border.color: parent.hovered ? "#4A90E2" : "#dddddd"
                    border.width: 1
                }
            }
        }

        // --- CALENDAR POPUP ---
        Popup {
            id: calendarPopup
            width: 320
            height: 360
            x: Math.round((parent.width - width) / 2)
            y: Math.round((parent.height - height) / 2)
            modal: true
            focus: true
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
            property bool selectingYear: false
            function isPastDate(y, m, d) {
                let cellDate = new Date(y, m, d)
                let today = new Date()
                today.setHours(0, 0, 0, 0)
                return cellDate < today
            }

            background: Rectangle {
                radius: 8
                color: "#ffffff"
                border.color: "#e0e0e0"
                border.width: 1
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 15

                // 1. PART: Month and Year Navigation
                RowLayout {
                    Layout.fillWidth: true

                    ToolButton {
                        text: "❮"
                        font.pixelSize: 16
                        visible: !calendarPopup.selectingYear
                        onClicked: grid.month = grid.month === 0 ? 11 : grid.month - 1
                    }

                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: calendarPopup.selectingYear ? "Yıl Seçin" : (grid.locale.standaloneMonthName(grid.month) + " " + grid.year)
                        font.pixelSize: 16
                        font.bold: true
                        color: "#333333"
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: calendarPopup.selectingYear = !calendarPopup.selectingYear
                        }
                    }

                    ToolButton {
                        text: "❯"
                        font.pixelSize: 16
                        visible: !calendarPopup.selectingYear
                        onClicked: grid.month = grid.month === 11 ? 0 : grid.month + 1
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: !calendarPopup.selectingYear

                    // 2. PART: DAY NAMES
                    DayOfWeekRow {
                        locale: grid.locale
                        Layout.fillWidth: true
                        delegate: Text {
                            text: model.shortName
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            color: "#777777"
                        }
                    }

                    // 3. PART: CALENDAR GRID
                    MonthGrid {
                        id: grid
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        month: new Date().getMonth()
                        year: new Date().getFullYear()
                        spacing: 2

                        delegate: Rectangle {
                            implicitWidth: 35
                            implicitHeight: 35
                            radius: 4
                            property bool isPast: calendarPopup.isPastDate(model.year, model.month, model.day)
                            color: (clickArea.containsMouse && !isPast) || model.today ? "#0078D7" : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: model.day
                                font.pixelSize: 14
                                color: parent.isPast ? "#dddddd" :
                                    (clickArea.containsMouse || model.today) ? "#ffffff"
                                    : (model.month === grid.month ? "#333333" : "#cccccc")
                            }

                            MouseArea {
                                id: clickArea
                                anchors.fill: parent
                                hoverEnabled: !parent.isPast
                                cursorShape: parent.isPast ? Qt.ForbiddenCursor : Qt.PointingHandCursor
                                onClicked: {
                                    if (parent.isPast) return;
                                    let d = model.day.toString().padStart(2, '0')
                                    let m = (model.month + 1).toString().padStart(2, '0')
                                    let y = model.year
                                    inputDate.text = d + "." + m + "." + y
                                    calendarPopup.close()
                                }
                            }
                        }
                    }
                }

                GridView {
                    id: yearGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: calendarPopup.selectingYear
                    cellWidth: width / 3
                    cellHeight: 45
                    clip: true

                    model: 30

                    delegate: Item {
                        width: yearGrid.cellWidth
                        height: yearGrid.cellHeight

                        property int currentIterYear: new Date().getFullYear() + index

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 4
                            radius: 6
                            color: yearMouse.containsMouse || grid.year === currentIterYear ? "#0078D7" : "#f5f5f5"

                            Text {
                                anchors.centerIn: parent
                                text: currentIterYear
                                font.pixelSize: 15
                                font.bold: grid.year === currentIterYear
                                color: yearMouse.containsMouse || grid.year === currentIterYear ? "#ffffff" : "#333333"
                            }

                            MouseArea {
                                id: yearMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    grid.year = currentIterYear
                                    calendarPopup.selectingYear = false
                                }
                            }
                        }
                    }
                }
            }
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

                    // 1. Reset previous errors and state
                    lastError = ""
                    generated = false

                    let userId = inputId.text
                    let password = inputPassword.text

                    // --- USER ID VALIDATIONS ---
                    // Rule 1: Minimum 6 characters
                    if (userId.length < 6) {
                        lastError = "Kullanıcı adı en az 6 karakter olmalıdır!"
                        return
                    }

                    // Rule 2: No special characters or spaces (Alphanumeric only)
                    let idRegex = /^[a-zA-Z0-9]+$/
                    if (!idRegex.test(userId)) {
                        lastError = "Kullanıcı adı özel karakter veya boşluk içeremez!"
                        return
                    }

                    // --- PASSWORD VALIDATIONS ---
                    // Rule 3: Minimum 8 characters
                    if (password.length < 8) {
                        lastError = "Şifre en az 8 karakter olmalıdır!"
                        return
                    }

                    // Rule 4: Must contain uppercase, lowercase, and special character
                    let hasUpperCase = /[A-Z]/.test(password)
                    let hasLowerCase = /[a-z]/.test(password)
                    let hasNumber = /[0-9]/.test(password)
                    let hasSpecialChar = /[^a-zA-Z0-9]/.test(password) // Anything that is NOT a letter or number

                    let missingReqs = []
                        if (!hasUpperCase) missingReqs.push("1 büyük harf")
                        if (!hasLowerCase) missingReqs.push("1 küçük harf")
                        if (!hasNumber) missingReqs.push("1 sayı")
                        if (!hasSpecialChar) missingReqs.push("1 özel karakter")

                    if (missingReqs.length > 0) {

                        lastError = "Şifre en az " + missingReqs.join(", ") + " içermelidir!"
                        return
                    }

                    var result = licenseManager.generateCustomLicense(inputId.text, inputPassword.text, inputDate.text)
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

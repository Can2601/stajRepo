import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Window {
    width: 480
    height: 400
    visible: true
    title: qsTr("Lisans Üretici")

    // Butona basılınca dolduracağımız değerler
    property string lastId: ""
    property string lastPassword: ""
    property string lastExpiryDate: ""
    property string lastFilePath: ""
    property bool generated: false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: qsTr("Lisans Üretici")
            font.pixelSize: 20
            font.bold: true
        }

        Button {
            text: qsTr("Lisans Üret")
            Layout.fillWidth: true
            onClicked: {
                // C++ tarafındaki LicenseManager.generateAndSave() çağrılıyor.
                // 365 gün geçerli bir lisans üretip dosyaya kaydediyor.
                var result = licenseManager.generateAndSave(365)
                lastId = result.id
                lastPassword = result.password
                lastExpiryDate = result.expiryDate
                lastFilePath = result.filePath
                generated = result.success
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 160
            visible: generated
            color: "#f0f0f0"
            radius: 8
            border.color: "#cccccc"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 6

                Label { text: qsTr("ID: ") + lastId; wrapMode: Text.WrapAnywhere }
                Label { text: qsTr("Şifre: ") + lastPassword }
                Label { text: qsTr("Son Kullanma Tarihi: ") + lastExpiryDate }
                Label {
                    text: qsTr("Kaydedildi: ") + lastFilePath
                    wrapMode: Text.WrapAnywhere
                    color: "#555555"
                    font.pixelSize: 11
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}

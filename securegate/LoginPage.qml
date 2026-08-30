import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs

Page {
    id: loginPage

    Rectangle {
        anchors.centerIn: parent
        color: "pink"
        anchors.fill: parent


        Column {
            anchors.centerIn: parent
            spacing: 10

            Text {
                text: "Welcome,"
                font.bold: true
                font.pixelSize: 20
            }
            Text {
                text: "Please enter your user ID and password."
                font.pixelSize: 15
            }

            Text {
                text: "ID:"
                font.bold: true
                font.pixelSize: 15
            }
            TextField {
                id: idField
                placeholderText: "ExampleID123"
                font.pixelSize: 13
                width: 260
                height: 35
            }

            Text {
                text: "Password:"
                font.bold: true
                font.pixelSize: 15
            }
            TextField {
                id: passwordField
                placeholderText: "ExamplePass123"
                echoMode: TextInput.Password
                font.pixelSize: 13
                width: 260
                height: 35
            }

            Button {
                text: "Login"
                font.bold: true
                font.pixelSize: 15
                width: 260
                onClicked: {
                    //license check
                    if (loginManager.getExpiryDate() === "") {
                        errorText.text = "--- Please select a valid license file first! ---"
                    }
                    //ID check
                    else if (idField.text === "") {
                        errorText.text = "--- ID is required. ---"
                    }
                    //password check
                    else if (passwordField.text === "") {
                        errorText.text = "--- Password is required. ---"
                    }
                    //login
                    else {
                        var success = loginManager.login(idField.text, passwordField.text);
                        console.log("login result:", success)

                        if (!success) {
                            errorText.text = "--- Invalid ID or password. ---"
                        }
                        else {
                            stackView.replace("DashboardPage.qml")
                        }
                    }
                }
            }

            Button {
                text: "Select License"
                font.bold: true
                font.pixelSize: 15
                width: 260
                onClicked: {
                    licenseDialog.open()
                }
            }

            FileDialog {
                id: licenseDialog
                title: "Select License File"
                nameFilters: ["License files (*.lic)"]

                onAccepted: {
                        var success = loginManager.loadLicense(licenseDialog.selectedFile)

                        if (success) {
                            errorText2.text = "License uploaded successfully. Expire Date: " + loginManager.getExpiryDate()
                        } else {
                            var expiry = loginManager.getExpiryDate()
                            if (expiry !== "") {
                                errorText2.text = "License EXPIRED on " + expiry + "!"
                            } else {
                                errorText2.text = "License is invalid."
                            }
                        }
                    }
            }

            Item {
                width: parent.width
                height: 23
                Text {
                    anchors.centerIn: parent
                    id: errorText2
                    text: ""
                    font.bold: true
                    font.pixelSize: 15
                }
            }

            Item {
                width: parent.width
                height: 23
                Text {
                    anchors.centerIn: parent
                    id: errorText
                    text: ""
                    font.bold: true
                    font.pixelSize: 15
                }
            }

        }
    }
}





import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs

ApplicationWindow {
    width: 380
    height: 370
    minimumWidth: 200
    minimumHeight: 250
    visible: true
    title: qsTr("Secure Gate")

    StackView {
        id: stackView
        anchors.fill: parent

        initialItem: loginPage
    }

    Component {
        id: loginPage

        Page{
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

                            if (idField.text === ""){
                                errorText.text = "--- ID is required. ---"
                            }
                            else if (passwordField.text === ""){
                                errorText.text = "--- Password is required. ---"
                            }
                            else{
                                var success = loginManager.login(idField.text, passwordField.text);
                                console.log("login result:", success)

                                if(!success){
                                    errorText.text = "--- Invalid ID or password. ---"
                                }
                                else{
                                    stackView.replace(loginPage,"SuccessPage.qml")
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
                            console.log("selected license:", selectedFile)
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
    }
}

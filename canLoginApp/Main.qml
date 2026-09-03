import QtQuick
import QtQuick.Controls.Basic

ApplicationWindow {
    id: window
    width:  900
    height: 620
    minimumWidth:  480
    minimumHeight: 420
    visible: true
    title: qsTr("Application – Login")

    color: "#0D1117"

    // ── Stack navigation ──────────────────────────────────────────────────────
    StackView {
        id: stack
        anchors.fill: parent

        // Start with the login page
        initialItem: loginComponent
    }

    // ── Login page ────────────────────────────────────────────────────────────
    Component {
        id: loginComponent

        LoginPage {
            onLoginSucceeded: function(userId) {
                window.title = qsTr("Application – %1").arg(userId)
                stack.push(welcomeComponent, { "userId": userId },
                           StackView.PushTransition)
            }

            onLoginFailed: function(reason) {
                // Already handled inside LoginPage (error banner shown).
                console.log("Login failed:", reason)
            }
        }
    }

    // ── Welcome page ──────────────────────────────────────────────────────────
    Component {
        id: welcomeComponent

        WelcomePage {
            onLogoutRequested: {
                window.title = qsTr("Application – Login")
                stack.pop()
            }
        }
    }
}

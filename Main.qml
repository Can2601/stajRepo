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

    // ── Background ────────────────────────────────────────────────
    color: "#0D1117"

    // ── Login page ────────────────────────────────────────────────
    LoginPage {
        id: loginPage
        anchors.fill: parent

        onLoginSucceeded: function(userId) {
            // TODO: navigate to your main application screen.
            // Example:
            //   window.title = qsTr("Application – %1").arg(userId)
            //   mainStack.push(dashboardComponent)
            console.log("Login succeeded for user:", userId)
        }

        onLoginFailed: function(reason) {
            // Already handled inside LoginPage (error banner shown).
            console.log("Login failed:", reason)
        }
    }
}

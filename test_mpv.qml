import QtQuick
import QtQuick.Window
import QtQuick.Controls
import com.stremio.libmpv 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 720
    title: "MPV Test"
    color: "#0c0b11"

    MpvObject {
        id: mpv
        anchors.fill: parent
        visible: true
    }

    Timer {
        interval: 2000
        running: true
        onTriggered: {
            console.log("Loading test stream...")
            mpv.command(["loadfile", "https://test-streams.mux.dev/x36xhzz/x36xhzz.m3u8"])
        }
    }

    Text {
        text: "If you see this text but no video, MpvObject rendering is broken"
        color: "white"
        font.pixelSize: 20
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 20
    }
}

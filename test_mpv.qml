import QtQuick
import QtQuick.Window
import com.stremio.libmpv 1.0

Window {
    id: root
    visible: true
    width: 1280
    height: 720
    title: "MPV Test"
    color: "black"

    MpvObject {
        id: mpv
        anchors.fill: parent
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

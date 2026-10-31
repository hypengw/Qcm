pragma ComponentBehavior: Bound
import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

Item {
    id: root

    enum Style {
        PageBlur,
        Cover
    }

    property int style: PlayBarBackground.PageBlur
    property Item sourceItem
    property url coverSource
    property bool changingTrack: false
    property bool active: true
    property real radius: 24
    readonly property bool usingCover: active && style === PlayBarBackground.Cover && cover.ready
    readonly property color surfaceColor: MD.MProp.color.surface_container
    readonly property color accentColor: cover.accentColor

    Item {
        id: composition
        anchors.fill: parent
        layer.enabled: root.active && cover.supported && root.radius > 0
        layer.effect: MD.RoundClip {
            radius: root.radius
            size: Qt.vector2d(composition.width, composition.height)
        }
        MD.BackdropSurface {
            objectName: "playBarBackdrop"
            anchors.fill: parent
            z: root.usingCover ? 0 : 2
            sourceItem: root.sourceItem
            active: root.active
            radius: cover.supported ? 0 : root.radius
            fallbackColor: root.surfaceColor
            tint: root.usingCover ? "transparent" : Qt.rgba(root.surfaceColor.r, root.surfaceColor.g, root.surfaceColor.b, 0.72)
        }
        Item {
            id: coverSurface
            anchors.fill: parent
            z: 1
            visible: root.active && root.style === PlayBarBackground.Cover && cover.supported && (cover.ready || root.coverSource.toString() !== "")
            opacity: 0.72
            QA.PlayingBackground {
                id: cover
                objectName: "playBarCover"
                anchors.fill: parent
                active: root.active && root.style === PlayBarBackground.Cover
                source: root.coverSource
                retainWhileLoading: root.changingTrack
                QA.PlayingBackgroundMask {
                    anchors.fill: parent
                    z: 3
                }
            }
        }
        Rectangle {
            anchors.fill: parent
            z: 3
            visible: root.usingCover
            color: Qt.rgba(0, 0, 0, 0.46)
        }
    }
}

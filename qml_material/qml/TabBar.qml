
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.TabBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    spacing: 1

    contentItem: ListView {
        model: control.contentModel
        currentIndex: control.currentIndex

        spacing: control.spacing
        orientation: ListView.Horizontal
        flickableDirection: Flickable.AutoFlickIfNeeded
        snapMode: ListView.SnapToItem

        highlightMoveDuration: 250
        highlightResizeDuration: 0
        highlightFollowsCurrentItem: true
        highlightRangeMode: ListView.ApplyRange
        preferredHighlightBegin: 48
        preferredHighlightEnd: width - 48

        highlight: Item {
            z: 2
            Rectangle {
                height: 2
                width: parent.width
                y: control.position === T.TabBar.Footer ? 0 : parent.height - height
                color: MD.Token.color.primary
            }
        }
    }

    background: Rectangle {
        color: MD.Token.color.surface

        //layer.enabled: control.Material.elevation > 0
        //layer.effect: MD.ElevationEffect {
        //    elevation: control.Material.elevation
        //    fullWidth: true
        //}

        MD.Divider {
            anchors.bottom: parent.bottom
        }
    }
}
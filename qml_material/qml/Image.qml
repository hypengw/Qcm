import QtQuick
import Qcm.Material as MD

Item {
    id: root
    // tl,tr,bl,br
    property var radius: [0]
    property alias asynchronous: image.asynchronous
    property alias autoTransform: image.autoTransform
    property alias cache: image.cache
    property alias currentFrame: image.currentFrame
    property alias fillMode: image.fillMode
    property alias frameCount: image.frameCount
    property alias horizontalAlignment: image.horizontalAlignment
    property alias mipmap: image.mipmap
    property alias mirror: image.mirror
    property alias mirrorVertically: image.mirrorVertically
    property alias progress: image.progress
    property alias source: image.source
    property alias sourceClipRect: image.sourceClipRect
    property alias sourceSize: image.sourceSize
    property alias status: image.status
    property alias verticalAlignment: image.verticalAlignment

    property alias paintedHeight: image.paintedHeight
    property alias paintedWidth: image.paintedWidth

    implicitHeight: {
        return sourceSize.height;
    }
    implicitWidth: {
        return sourceSize.width;
    }

    MD.MatProp.elevation: MD.Token.elevation.level0
    layer.enabled: root.status === Image.Ready && root.paintedHeight > 0 && root.MD.MatProp.elevation != MD.Token.elevation.level0
    layer.effect: MD.RoundedElevationEffect {
        elevation: root.MD.MatProp.elevation
    }

    Image {
        id: image
        anchors.fill: parent
        cache: true
        smooth: true
        fillMode: Image.PreserveAspectCrop


        layer.enabled: true
        layer.effect: MD.RoundClip {
            radius: root.radius
            size: root.height
        }
    }
}
import QtQuick
import QExtra as QE
import Qcm.App as QA
import Qcm.Material as MD

Item {
    id: root

    property size displaySize: Qt.size(0, 0)
    property bool fixedSize: true
    property int radius: 0
    property MD.corners corners: MD.Util.corners(radius)
    property int elevation: MD.Token.elevation.level0
    property alias inner: image
    property alias source: image.source
    property alias sourceSize: image.sourceSize
    property alias maxSize: image.maxSize
    property alias status: image.status
    property alias progress: image.progress
    property alias errorString: image.errorString
    property alias cache: image.cache
    property alias mipmap: image.mipmap
    property alias fillMode: image.fillMode
    property alias retainWhileLoading: image.retainWhileLoading
    property alias mirror: image.mirror
    property alias horizontalAlignment: image.horizontalAlignment
    property alias verticalAlignment: image.verticalAlignment
    property alias paintedWidth: image.paintedWidth
    property alias paintedHeight: image.paintedHeight

    implicitWidth: displaySize.width > 0 ? displaySize.width : image.sourceSize.width
    implicitHeight: displaySize.height > 0 ? displaySize.height : image.sourceSize.height

    MD.Elevation {
        anchors.fill: parent
        elevation: root.elevation
        corners: root.corners
        opacity: image.status === QE.Image.Ready && image.paintedWidth > 0 ? 1 : 0
    }
    QE.Image {
        id: image
        anchors.fill: parent
        fillMode: QE.Image.PreserveAspectCrop
        retainWhileLoading: true
        sourceSize: {
            if (root.displaySize.width <= 0 || root.displaySize.height <= 0)
                return Qt.size(-1, -1);
            const size = root.fixedSize ? root.displaySize : QA.App.bound_image_size(root.displaySize);
            return Qt.size(Math.ceil(size.width * Screen.devicePixelRatio),
                           Math.ceil(size.height * Screen.devicePixelRatio));
        }
        layer.enabled: true
        layer.effect: MD.RoundClip {
            corners: root.corners
            size: Qt.vector2d(root.width, root.height)
        }
    }
}

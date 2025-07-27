import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.ListItem {
    id: root
    property var itemId: model.itemId
    property string image

    width: ListView.view.width
    maximumLineCount: 2

    radius: MD.Token.shape.corner.large
    corners: MD.Util.corners(0, index + 1 == count ? root.radius : 0)

    leader: QA.Image {
        radius: 8
        source: root.image
        implicitWidth: displaySize.width
        implicitHeight: displaySize.height

        displaySize: Qt.size(48, 48)
    }
    rightPadding: 0
    trailing: MD.IconButton {
        MD.MProp.textColor: MD.Token.color.on_surface_variant
        icon.name: MD.Token.icon.more_vert
        onClicked: {
            QA.Action.openItemMenu(root.itemId, this);
        }
    }
}

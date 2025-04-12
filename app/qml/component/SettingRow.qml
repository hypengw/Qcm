import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

MD.ListItem {
    id: root
    Layout.fillWidth: true
    font.capitalization: Font.Capitalize
    corners: {
        let s = root.start ? MD.Token.shape.corner.medium : 0;
        let e = root.end ? MD.Token.shape.corner.medium : 0;
        return MD.Util.corner(s, e);
    }
    property bool canInput: true
    property bool start: false
    property bool end: false

    mdState.backgroundColor: MD.MProp.color.surface_container

    MD.InputBlock {
        when: !root.canInput
        target: root
    }

    Component.onCompleted:
    // if (root.actionItem?.clicked)
    //     clicked.connect(root.actionItem.clicked);
    // if (root.actionItem?.checkable)
    //     clicked.connect(root.actionItem.toggle);
    {}
}

import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

MD.ListItem {
    id: root
    index: -1
    model: null
    Layout.fillWidth: true
    font.capitalization: Font.Capitalize
    corners: {
        let s = root.start ? MD.Token.shape.corner.medium : 0;
        let e = root.end ? MD.Token.shape.corner.medium : 0;
        return MD.Util.corners(s, e);
    }
    property bool canInput: true
    property bool start: false
    property bool end: false

    mdState.backgroundColor: MD.MProp.color.surface_container

    MD.InputBlock {
        when: !root.canInput
        target: root
    }

    Component.onCompleted: {
        if (root.trailing?.clicked)
            clicked.connect(root.trailing.clicked);
        if (root.trailing?.checkable)
            clicked.connect(root.trailing.toggle);
    }
}

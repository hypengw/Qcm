import QtQuick
import QtQuick.Layouts

import Qcm.Material as MD

Rectangle {
    id: root
    property bool full: true

    Binding {
        root.Layout.fillWidth: true
        when: full && isLayout(root.parent)
    }

    Binding {
        root.anchors.left: root.parent.left
        root.anchors.right: root.parent.right
        when: full && !isLayout(root.parent)
    }

    function isLayout(p) {
        return (p instanceof RowLayout) || (p instanceof GridLayout) || (p instanceof ColumnLayout);
    }

    implicitHeight: 1
    height: 1
    color: MD.Token.color.outline_variant
}
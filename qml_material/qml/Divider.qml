import QtQuick
import QtQuick.Layouts

import Qcm.Material as MD

Rectangle {
    id: root
    property bool full: true
    property int orientation: {
        if(parent instanceof RowLayout)
            return Qt.Vertical
        else return Qt.Horizontal
    }
    property int size: 1

    Binding {
        root.Layout.fillWidth: true
        root.implicitHeight: root.size
        root.height: root.size
        when: full && isLayout(root.parent) && orientation === Qt.Horizontal
    }

    Binding {
        root.anchors.left: root.parent.left
        root.anchors.right: root.parent.right
        root.implicitHeight: root.size
        root.height: root.size
        when: full && !isLayout(root.parent) && orientation === Qt.Horizontal
    }

    Binding {
        root.Layout.fillHeight: true
        root.implicitWidth: root.size
        root.width: root.size
        when: full && isLayout(root.parent) && orientation !== Qt.Horizontal
    }

    Binding {
        root.anchors.top: root.parent.top
        root.anchors.bottom: root.parent.bottom
        root.implicitWidth: root.size
        root.width: root.size
        when: full && !isLayout(root.parent) && orientation !== Qt.Horizontal
    }

    function isLayout(p) {
        return (p instanceof RowLayout) || (p instanceof GridLayout) || (p instanceof ColumnLayout);
    }
    color: MD.Token.color.outline_variant
}
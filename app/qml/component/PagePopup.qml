import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Dialog {
    id: root

    MD.MatProp.textColor: MD.Token.color.on_surface
    MD.MatProp.backgroundColor: MD.Token.color.surface

    property bool fillHeight: false
    property var props: Object()
    required property string source

    title: loader.item.title
    titleCapitalization: loader.item.font.capitalization
    width: Math.min(400, parent.width)

    //    readonly property int contentMaxHeight: Math.max(maxHeight - headHeight, 0)
    //    readonly property int headHeight: head_pane.implicitHeight
    readonly property int maxHeight: parent.height * 0.8

    height: Math.min(implicitHeight, maxHeight)
    modal: true
    parent: Overlay.overlay
    // width: parent.width / 2
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)

    Binding on height {
        value: maxHeight
        when: fillHeight
    }

    onSourceChanged: {
        loader.setSource(source, props);
    }

    contentItem: Loader {
        id: loader
    }

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
}

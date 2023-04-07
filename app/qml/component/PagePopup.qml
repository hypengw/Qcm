import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."

MPopup {
    id: root

    property bool fillHeight: false
    property var props
    required property string source

    title: loader.item.title
    width: 400

    Binding on height  {
        value: maxHeight
        when: fillHeight
    }

    onSourceChanged: {
        loader.setSource(source, props ? props : {});
    }

    Loader {
        id: loader
        Layout.fillHeight: true
        Layout.fillWidth: true
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."

MPopup {
    id: root

    required property string source
    required property var props
    property bool fillHeight: false

    width: 400
    title: loader.item.title
    onSourceChanged: {
        loader.setSource(source, props);
    }

    Loader {
        id: loader

        Layout.fillWidth: true
        Layout.fillHeight: true
    }

    Binding on height {
        when: fillHeight
        value: maxHeight
    }

}

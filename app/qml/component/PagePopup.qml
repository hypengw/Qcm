import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."

MPopup {
    id: root

    required property string source
    required property var props

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

}

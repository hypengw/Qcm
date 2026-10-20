pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.EmbedChip {
    id: root
    property var options: []
    property string value
    signal valueChangeRequested(string value)

    text: {
        for (const option of options) {
            if (option.value === value)
                return option.name;
        }
        return "";
    }
    trailingIconName: MD.Token.icon.arrow_drop_down
    onClicked: popup.open()

    readonly property MD.Menu popup: MD.Menu {
        parent: root
        anchorItem: root
        model: root.options
        contentDelegate: MD.MenuItem {
            required property var model
            text: model.name
            selected: model.value === root.value
            onClicked: {
                const value = model.value;
                root.popup.close();
                root.valueChangeRequested(value);
            }
        }
    }
}

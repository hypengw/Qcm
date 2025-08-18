pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD
import Qcm.App as QA

MD.Menu {
    id: root
    signal selected(int type)
    contentDelegate: MD.MenuItem {
        required property var model
        text: model.name
        onClicked: {
            root.selected(model.type);
        }
    }
}

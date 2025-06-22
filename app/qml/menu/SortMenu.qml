pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD
import QtQml.Models

MD.Menu {
    id: root
    contentDelegate: MD.MenuItem {
        required property var model
        text: model.name
        icon.name: {
            const m = root.model;
            if (m.currentIndex == model.index) {
                return m.asc ? MD.Token.icon.arrow_upward : MD.Token.icon.arrow_downward;
            } else {
                return ' ';
            }
        }
        onClicked: {
            const m = root.model;
            if (m.currentIndex == model.index) {
                m.asc = !m.asc;
            } else {
                m.currentIndex = model.index;
            }
            root.close();
        }
    }
}

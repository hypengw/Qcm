import QtQuick
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 16

    MD.MatProp.backgroundColor: item_search.focus ? item_search.MD.MatProp.backgroundColor : MD.Token.color.surface

    ColumnLayout {
        anchors.fill: parent
        MD.SearchBar {
            id: item_search
            Layout.fillWidth: true
        }
    }
}
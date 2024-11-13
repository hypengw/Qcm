import QtQuick
import QtQuick.Controls.Basic

import Qcm.App as QA
import Qcm.Material as MD

Action {
    id: root
    icon.name: MD.Token.icon.person
    text: qsTr('go to artist')

    property var getItemIds: null

    onTriggered: {
        if (!getItemIds)
            return;
        const itemIds = root.getItemIds();
        if (itemIds.length === 1) {
            QA.Action.route_by_id(itemIds[0]);
        } else {
            MD.Util.show_popup('qrc:/Qcm/App/qml/dialog/GoToArtistDialog.qml', {
                "itemIds": itemIds
            }, QA.Global.main_win.Overlay.overlay);
        }
    }
}

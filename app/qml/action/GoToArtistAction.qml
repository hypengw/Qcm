import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    icon.name: MD.Token.icon.person
    text: qsTr('go to artist')

    property var getItemIds: null

    onTriggered: {
        if (!getItemIds)
            return;
        const itemIds = root.getItemIds();
        if (itemIds.length === 1) {
            QA.Action.routeItem(itemIds[0]);
        } else {
            MD.Util.showPopup('qrc:/Qcm/App/qml/dialog/GoToArtistDialog.qml', {
                "itemIds": itemIds
            }, QA.Global.main_win);
        }
    }
}

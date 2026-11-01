import QtQuick

import Qcm.App as QA
import Qcm.Msg as QM
import Qcm.Material as MD

MD.Action {
    id: root
    icon.name: MD.Token.icon.person
    text: qsTr('go to artist')

    property QM.artistIds artistIds
    property bool albumArtist: false

    onTriggered: {
        const itemIds = QM.Ids.artistItems(root.artistIds);
        if (itemIds.length === 0) return;
        if (itemIds.length === 1) {
            QA.Action.routeItem(itemIds[0], {"albumArtist": root.albumArtist});
        } else {
            const msg = QA.Util.routeMsg();
            msg.dst = 'Qcm.App/GoToArtistDialog';
            msg.props = {
                'artistIds': root.artistIds,
                'albumArtist': root.albumArtist
            };
            QA.Action.openPopup(msg);
        }
    }
}

import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    required property QA.item_id itemId
    icon.name: MD.Token.icon.comment
    text: qsTr('Comments')
    enabled: itemId.valid && [QA.Enum.ItemSong, QA.Enum.ItemAlbum, QA.Enum.ItemMix].includes(itemId.type)
    onTriggered: {
        const msg = QA.Util.routeMsg();
        msg.dst = 'Qcm.App/CommentPopup';
        msg.props = {
            itemId: root.itemId
        };
        QA.Action.openPopup(msg);
    }
}

import QtQuick

import Qcm.App as QA
import Qcm.Msg as QM
import Qcm.Material as MD

MD.Action {
    id: root
    property alias itemId: m_query.itemId
    property QM.item_id sourceId

    enabled: root.itemId.valid
    icon.name: MD.Token.icon.play_arrow
    text: qsTr('play')
    busy: m_query.querying
    closeMenu: false

    onTriggered: m_query.reload()

    QA.PlayQuery {
        id: m_query
        onFinished: MD.Util.closeMenuOn(root)
    }
}

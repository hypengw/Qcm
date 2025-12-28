import QtQuick
import QtQml

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    property alias itemId: m_qr.itemId

    icon.name: MD.Token.icon.sync
    text: qsTr('sync')
    onTriggered: m_qr.reload()
    signal finished()

    QA.SyncItemQuery {
        id: m_qr
        onFinished: root.finished()
    }
}

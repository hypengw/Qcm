import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    property alias ids: m_qr.ids
    icon.name: MD.Token.icon.delete
    text: qsTr('delete')
    onTriggered: {
        m_qr.reload();
    }

    QA.DeleteMixQuery {
        id: m_qr
    }
}

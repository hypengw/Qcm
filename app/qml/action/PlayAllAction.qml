import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    icon.name: MD.Token.icon.play_arrow
    text: qsTr('play all')

    property alias filters: m_qr.filters
    busy: m_qr.querying

    QA.PlayAllQuery {
        id: m_qr
    }

    onTriggered: {
        m_qr.reload();
    }
}

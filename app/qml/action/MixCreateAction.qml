import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    icon.name: MD.Token.icon.add
    text: qsTr('create mix')
    onTriggered: {
        const msg = QA.Util.routeMsg();
        msg.dst = 'Qcm.App/MixCreateDialog';
        msg.props = {};
        QA.Action.openPopup(msg);
    }
}

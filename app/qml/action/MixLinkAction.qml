import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    icon.name: MD.Token.icon.add_link
    text: qsTr('link mix')
    onTriggered: {
        const msg = QA.Util.routeMsg();
        msg.dst = 'Qcm.App/MixLinkDialog';
        msg.props = {};
        QA.Action.openPopup(msg);
    }
}

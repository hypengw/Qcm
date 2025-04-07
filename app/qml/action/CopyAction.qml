import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    text: qsTr('copy')
    icon.name: MD.Token.icon.link
    property var getCopyString: function () {
        return '';
    }

    onTriggered: {
        QA.Clipboard.text = getCopyString();
        QA.Action.toast(qsTr("Copied to clipboard"), 2000);
    }
}

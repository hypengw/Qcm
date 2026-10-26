import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    property string copyText: ""
    property var getCopyString: null
    signal copied()
    readonly property string value: {
        const text = typeof getCopyString === "function" ? getCopyString() : copyText;
        return typeof text === "string" ? text : "";
    }

    text: qsTr('Copy')
    icon.name: MD.Token.icon.content_copy
    enabled: value.trim().length > 0 && QA.Clipboard.supported && !busy
    closeMenu: false
    onTriggered: {
        if (!QA.Clipboard.copyText(root.value)) {
            QA.Action.toast(qsTr("Clipboard is unavailable"), 2000);
            return;
        }
        root.copied();
        QA.Action.toast(qsTr("Copied to clipboard"), 2000);
    }
}

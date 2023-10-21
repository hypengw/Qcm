import QtQuick

import Qcm.Material as MD

ListView {
    id: root

    property bool busy: false

    footer: MD.ListBusyFooter {
        running: root.busy
        width: ListView.view.width
    }
}

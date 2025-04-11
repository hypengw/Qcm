import QtQuick
import Qcm.Material as MD
import Qcm.App as QA

MD.Label {
    visible: QA.App.providerStatus.syncing
    text: 'Syncing database, please wait.'
    maximumLineCount: 2
    opacity: 0.8
}

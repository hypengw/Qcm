import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Dialog {
    id: root
    title: qsTr('Create A Mix')
    standardButtons: MD.Dialog.Ok | MD.Dialog.Cancel

    onAccepted: {
        item_input.validator = item_valid;
        if (item_input.acceptableInput) {
            m_query.reload();
        } else {
            item_input.focus = true;
        }
    }
    onRejected: MD.Util.closePopup(root)

    ColumnLayout {
        anchors.fill: parent
        spacing: 24

        ColumnLayout {
            MD.TextField {
                id: item_input
                Layout.fillWidth: true
                placeholderText: qsTr('mix name')
            }
        }
        RegularExpressionValidator {
            id: item_valid
            regularExpression: /.+/
        }

        QA.MixCreateQuery {
            id: m_query
            name: item_input.text
            onFinished: {
                // QA.App.playqueueCreated();
                MD.Util.closePopup(root);
            }
        }
    }
}

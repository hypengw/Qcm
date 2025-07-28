import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.ListItem {
    id: root
    index: -1
    model: null
    property string description
    visible: root.description.length > 0

    contentItem: Item {
        implicitHeight: item_text.implicitHeight
        MD.Text {
            id: item_text
            anchors.left: parent.left
            anchors.right: parent.right
            maximumLineCount: 3
            typescale: MD.Token.typescale.body_medium
            text: root.description
        }
    }

    onClicked: {
        const msg = Util.routeMsg();
        msg.dst = 'Qcm.App/PagePopup';
        msg.props = {
            source: 'Qcm.App/DescriptionPage',
            props: {
                text: description
            }
        };
        QA.Action.openPopup(msg);
    }
}

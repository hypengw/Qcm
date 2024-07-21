import QtQuick
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.ListItem {
    id: root
    property string description
    visible: !!root.description

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
        QA.Global.show_page_popup('qrc:/Qcm/App/qml/page/DescriptionPage.qml', {
                "text": description
            });
    }
}
pragma ComponentBehavior: Bound
import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Dialog {
    id: root
    title: qsTr(`add to mix`)
    padding: 0
    standardButtons: MD.Dialog.Cancel

    required property var songId

    MD.ListView {
        id: m_view
        anchors.fill: parent
        expand: true
        topMargin: 8
        bottomMargin: 8

        busy: m_qr.status === QA.enums.Querying
        model: m_qr.data
        header: MD.ListItem {
            font.capitalization: Font.Capitalize
            width: ListView.view.width
            leader: Item {
                implicitHeight: 48
                implicitWidth: 48
                MD.Icon {
                    anchors.centerIn: parent
                    name: MD.Token.icon.add
                }
            }
            corners: indexCorners(0, 1 + ListView.view.count, 16)
            action: QA.MixCreateAction {}
        }
        delegate: MD.ListItem {
            required property int index
            required property var model
            text: model.name
            supportText: `${model.trackCount} songs`
            width: ListView.view.width
            maximumLineCount: 2
            leader: QA.Image {
                radius: 8
                source: QA.Util.image_url(model.picUrl)
                sourceSize.height: 48
                sourceSize.width: 48
            }
            corners: indexCorners(index + 1, count + 1, 16)
            onImplicitWidthChanged: {
                const v = ListView.view;
                v.implicitWidth = Math.max(v.implicitWidth, implicitWidth);
            }
            onClicked: {
                m_qr_manipulate.mixId = model.itemId;
                m_qr_manipulate.itemIds = [root.songId];
                m_qr_manipulate.reload();
            }
        }

        QA.UserMixQuery {
            id: m_qr
            Component.onCompleted: reload()
        }

        QA.MixManipulateQuery {
            id: m_qr_manipulate
            oper: QA.enums.ManipulateMixAdd

            onFinished: {
                MD.Util.closePopup(root);
            }
        }
    }
}

pragma ComponentBehavior: Bound
import QtQuick
import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

MD.Dialog {
    id: root
    title: qsTr(`add to mix`)
    padding: 0
    standardButtons: MD.Dialog.Cancel

    property int oper: QM.MixManipulateOper.MIX_MANIPULATE_OPER_ADD_SONGS
    property list<QA.item_id> songIds
    property list<QA.item_id> albumIds

    QA.MixesQuery {
        id: m_qr_mixes
        Component.onCompleted: reload()
    }

    QA.MixManipulateQuery {
        id: m_qr
        oper: root.oper
        songIds: root.songIds
        albumIds: root.albumIds

        onFinished: {
            MD.Util.closePopup(root);
        }
    }

    MD.VerticalListView {
        id: m_view
        anchors.fill: parent
        expand: true
        topMargin: 8
        bottomMargin: 8

        busy: m_qr_mixes.status === QA.Enum.Querying
        model: m_qr_mixes.data
        header: Column {
            width: ListView.view.width
            MD.ActionToolBar {
                width: parent.width
                actions: [
                    QA.MixCreateAction {},
                    QA.MixLinkAction {}
                ]
            }
        }
        delegate: MD.ListItem {
            text: model.name
            supportText: `${model.trackCount} songs`
            width: ListView.view.width
            maximumLineCount: 2
            /*
            leader: QA.Image {
                radius: 8
                source: QA.Util.image_url(model.picUrl)
                sourceSize.height: 48
                sourceSize.width: 48
            }
            */
            corners: MD.Util.listCorners(index, count, 16)
            onImplicitWidthChanged: {
                const v = ListView.view;
                v.implicitWidth = Math.max(v.implicitWidth, implicitWidth);
            }
            onClicked: {
                m_qr.mixId = model.itemId;
                m_qr.reload();
            }
        }
    }
}

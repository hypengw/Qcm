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
    property QM.songIds songIds
    property QM.albumIds albumIds

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

    MD.VerticalLazyList {
        id: m_view
        anchors.fill: parent
        expand: true
        extraTopMargin: 8
        extraBottomMargin: 8

        busy: m_qr_mixes.status === QA.Enum.Querying
        source: QA.ItemModelSource {
            model: m_qr_mixes.data
        }
        header: Column {
            width: m_view.contentWidth
            MD.ActionToolBar {
                width: parent.width
                actions: [
                    QA.MixCreateAction {},
                    QA.MixLinkAction {}
                ]
            }
        }
        delegate: MD.ListItem {
            required property MD.LazyRow row
            itemRow: row
            text: model.name
            supportText: `${model.trackCount} songs`

            maximumLineCount: 2
            /*
            leader: QA.Image {
                radius: 8
                source: QA.Util.image_url(model.picUrl)
                displaySize: Qt.size(48, 48)
            }
            */
            corners: MD.Util.listCorners(index, count, 16)
            onImplicitWidthChanged: {
                const v = m_view;
                v.implicitWidth = Math.max(v.implicitWidth, implicitWidth);
            }
            onClicked: {
                m_qr.mixId = model.itemId;
                m_qr.reload();
            }
        }
    }
}

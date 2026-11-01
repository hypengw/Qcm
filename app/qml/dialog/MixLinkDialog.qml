pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

MD.Dialog {
    id: root
    title: qsTr('Link A Mix')
    standardButtons: MD.Dialog.Ok | MD.Dialog.Cancel

    onAccepted: {}

    QA.RemoteMixesQuery {
        id: m_qr_mixes
        asc: false
        property QM.remoteMixFilter filter1
        property QM.remoteMixFilter filter2
        filter1.typeFilter: {
            const f = QA.Util.typeStringFilter();
            f.value = "user";
            f.condition = QM.TypeCondition.TYPE_CONDITION_IS;
            return f;
        }
        filter2.localTypeFilter: {
            const f = QA.Util.typeFilter();
            f.value = QM.MixType.MIX_TYPE_LINK;
            f.condition = QM.TypeCondition.TYPE_CONDITION_IS_NOT;
            return f;
        }
        filters: [filter1, filter2]
    }

    QA.LinkMixQuery {
        id: m_qr_link
        onFinished: {
            root.accept();
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
                actions: []
            }
        }
        delegate: MD.ListItem {
            required property MD.LazyRow row
            itemRow: row
            text: model.name
            supportText: `${model.trackCount} songs`

            maximumLineCount: 2
            leader: QA.Image {
                radius: 8
                source: QA.Util.image_url(model.itemId)
                displaySize: Qt.size(48, 48)
            }
            corners: MD.Util.listCorners(index, count, 16)
            onImplicitWidthChanged: {
                const v = m_view;
                v.implicitWidth = Math.max(v.implicitWidth, implicitWidth);
            }
            onClicked: {
                m_qr_link.ids = QM.Ids.mixIds([model.itemId]);
                m_qr_link.reload();
            }
        }
    }
}

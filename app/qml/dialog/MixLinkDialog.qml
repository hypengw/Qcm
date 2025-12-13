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
                actions: []
            }
        }
        delegate: MD.ListItem {
            text: model.name
            supportText: `${model.trackCount} songs`
            width: ListView.view.width
            maximumLineCount: 2
            leader: QA.Image {
                radius: 8
                source: QA.Util.image_url(model.itemId)
                sourceSize.height: 48
                sourceSize.width: 48
            }
            corners: MD.Util.listCorners(index, count, 16)
            onImplicitWidthChanged: {
                const v = ListView.view;
                v.implicitWidth = Math.max(v.implicitWidth, implicitWidth);
            }
            onClicked: {
                m_qr_link.ids = [model.itemId];
                m_qr_link.reload();
            }
        }
    }
}

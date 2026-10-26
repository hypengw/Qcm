pragma ComponentBehavior: Bound
import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    font.capitalization: Font.Capitalize
    title: qsTr('Provider Manage')
    bottomPadding: radius
    scrolling: !m_view.atYBeginning

    actions: [
        MD.Action {
            text: qsTr('Add')
            tooltip: qsTr("Add provider")
            icon.name: MD.Token.icon.add
            onTriggered: {
                root.MD.MProp.page.pushItem('qrc:/Qcm/App/qml/page/ProviderAddPage.qml');
            }
        }
    ]

    QA.DeleteProviderQuery {
        id: m_delete_query
    }

    MD.VerticalListView {
        id: m_view
        anchors.fill: parent
        leftMargin: 16
        rightMargin: 16
        spacing: 2
        expand: true
        model: QA.App.providerStatus

        delegate: QA.SettingRow {
            required index
            required model
            width: ListView.view.contentWidth
            start: index === 0
            end: index === m_view.count - 1
            font.capitalization: Font.MixedCase
            leader: MD.IconSvg {
                sourceData: QA.App.providerStatus.svg(index)
                size: 24
            }
            text: model.name
            trailing: MD.BusyIconButton {
                icon.name: MD.Token.icon.close
                busy: {
                    const query = m_delete_query;
                    return query.providerId == model.itemId && query.querying;
                }
                onClicked: {
                    const query = m_delete_query;
                    query.providerId = model.itemId;
                    query.reload();
                }
            }

            onClicked: {
                root.MD.MProp.page.pushItem('qrc:/Qcm/App/qml/page/edit/ProviderEditPage.qml', {
                    itemId: model.itemId
                });
            }
        }

    }
}

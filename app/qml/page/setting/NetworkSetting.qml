pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

QA.SettingBasePage {
    id: root
    title: qsTr("Network")
    actions: [
        MD.Action {
            text: qsTr("Add proxy")
            icon.name: MD.Token.icon.add
            onTriggered: root.MD.MProp.page.pushItem("Qcm.App/ProxyEditPage")
        }
    ]
    QA.NetworkSettingsQuery {
        id: settingsQuery
        Component.onCompleted: reload()
    }
    Connections {
        target: root.MD.PageStack
        function onStatusChanged() {
            if (root.MD.PageStack.status === MD.PageStack.Active && !settingsQuery.querying)
                settingsQuery.reload();
        }
    }
    ColumnLayout {
        width: parent.width
        height: implicitHeight
        spacing: 12
        MD.Label {
            Layout.fillWidth: true
            text: qsTr("Assign proxies to providers. Unassigned providers connect directly.")
            font.capitalization: Font.MixedCase
            wrapMode: Text.Wrap
        }
        MD.BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            visible: settingsQuery.querying
            running: visible
        }
        MD.Label {
            Layout.fillWidth: true
            visible: settingsQuery.status === QA.Enum.Finished && settingsQuery.proxies.length === 0
            text: qsTr("No proxies. Use Add to create one.")
            font.capitalization: Font.MixedCase
            wrapMode: Text.Wrap
        }
        QA.SettingSection {
            id: proxySection
            Layout.fillWidth: true
            title: qsTr("Proxies")
            visible: settingsQuery.proxies.length > 0
            spacing: 2
            Repeater {
                model: settingsQuery.proxies
                onItemAdded: proxySection.columnChanged()
                onItemRemoved: proxySection.columnChanged()
                delegate: QA.SettingRow {
                    id: proxyRow
                    objectName: "proxy/" + modelData.id
                    required property var modelData
                    text: modelData.name
                    font.capitalization: Font.MixedCase
                    below: MD.Label {
                        text: proxyRow.modelData.display_url + "\n" + qsTr("%n provider(s)", "", proxyRow.modelData.provider_ids.length)
                        typescale: MD.Token.typescale.body_medium
                        font.capitalization: Font.MixedCase
                        color: proxyRow.mdState.supportTextColor
                        wrapMode: Text.Wrap
                    }
                    trailing: MD.Icon {
                        name: MD.Token.icon.chevron_right
                    }
                    onClicked: root.MD.MProp.page.pushItem("Qcm.App/ProxyEditPage", {
                        proxyId: modelData.id
                    })
                }
            }
        }
        MD.Label {
            Layout.fillWidth: true
            visible: settingsQuery.error.length > 0
            text: settingsQuery.error
            color: MD.Token.color.error
            wrapMode: Text.Wrap
        }
        MD.Button {
            visible: settingsQuery.status === QA.Enum.Error
            text: qsTr("Retry")
            onClicked: settingsQuery.reload()
        }
    }
}

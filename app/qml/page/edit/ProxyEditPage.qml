pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

QA.SettingBasePage {
    id: root
    property string proxyId: ""
    property var selectedIds: []
    readonly property var original: settingsQuery.proxies.find(proxy => proxy.id === proxyId) ?? null
    readonly property bool available: settingsQuery.status === QA.Enum.Finished && (!proxyId || original !== null)
    readonly property bool modified: nameField.text !== (original?.name ?? "") || urlField.text !== (original?.url ?? "") || JSON.stringify(selectedIds.slice().sort((a, b) => a - b)) !== JSON.stringify((original?.provider_ids ?? []).slice().sort((a, b) => a - b))
    title: proxyId ? qsTr("Edit Proxy") : qsTr("Add Proxy")
    actions: [
        MD.Action {
            text: qsTr("Reset")
            icon.name: MD.Token.icon.restart_alt
            enabled: root.available && root.modified && !updateQuery.querying
            onTriggered: root.resetDraft()
        },
        MD.Action {
            text: qsTr("Delete")
            icon.name: MD.Token.icon.delete
            visible: root.proxyId.length > 0
            enabled: root.available && !updateQuery.querying
            onTriggered: root.saveDraft(true)
        },
        MD.Action {
            text: qsTr("Save")
            icon.name: MD.Token.icon.save
            busy: updateQuery.querying
            enabled: root.available && root.modified && !updateQuery.querying && nameField.text.trim().length > 0 && urlField.text.trim().length > 0
            onTriggered: root.saveDraft(false)
        }
    ]
    function resetDraft() {
        nameField.text = original?.name ?? "";
        urlField.text = original?.url ?? "";
        selectedIds = (original?.provider_ids ?? []).slice();
    }
    function selectProvider(id, selected) {
        id = Number(id);
        selectedIds = selected ? [...selectedIds.filter(value => value !== id), id] : selectedIds.filter(value => value !== id);
    }
    function assignedName(id) {
        const proxy = settingsQuery.proxies.find(proxy => proxy.provider_ids.includes(Number(id)));
        return proxy ? qsTr("Current proxy: %1").arg(proxy.name) : qsTr("Direct connection");
    }
    function saveDraft(remove) {
        if (!available || updateQuery.querying)
            return;
        let proxies = JSON.parse(JSON.stringify(settingsQuery.proxies));
        if (remove) {
            proxies = proxies.filter(proxy => proxy.id !== proxyId);
        } else {
            const draft = {
                id: proxyId,
                name: nameField.text,
                url: urlField.text,
                provider_ids: selectedIds.slice()
            };
            const index = proxies.findIndex(proxy => proxy.id === proxyId);
            for (const proxy of proxies)
                if (proxy.id !== proxyId)
                    proxy.provider_ids = proxy.provider_ids.filter(id => !selectedIds.includes(id));
            if (index >= 0)
                proxies[index] = draft;
            else
                proxies.push(draft);
        }
        updateQuery.save(proxies, settingsQuery.revision);
    }
    QA.NetworkSettingsQuery {
        id: settingsQuery
        Component.onCompleted: reload()
        onStatusChanged: if (status === QA.Enum.Finished)
            root.resetDraft()
    }
    QA.UpdateNetworkSettingsQuery {
        id: updateQuery
        onStatusChanged: if (status === QA.Enum.Finished)
            root.MD.MProp.page.pop()
    }
    ColumnLayout {
        width: parent.width
        height: implicitHeight
        spacing: 12
        enabled: !updateQuery.querying
        QA.SettingSection {
            Layout.fillWidth: true
            enabled: root.available
            title: qsTr("Proxy")
            spacing: 12
            MD.TextField {
                id: nameField
                objectName: "proxyName"
                Layout.fillWidth: true
                type: MD.Enum.TextFieldFilled
                placeholderText: qsTr("Name")
            }
            MD.TextField {
                id: urlField
                objectName: "proxyUrl"
                Layout.fillWidth: true
                type: MD.Enum.TextFieldFilled
                placeholderText: qsTr("Proxy URL")
                inputMethodHints: Qt.ImhUrlCharactersOnly
            }
            MD.Label {
                Layout.fillWidth: true
                text: qsTr("HTTP, HTTPS, SOCKS5 or SOCKS5H, for example http://127.0.0.1:7890. Changes apply to subsequent requests.")
                font.capitalization: Font.MixedCase
                wrapMode: Text.Wrap
            }
        }
        QA.SettingSection {
            id: providersSection
            Layout.fillWidth: true
            enabled: root.available
            title: qsTr("Providers")
            spacing: 2
            MD.Label {
                Layout.fillWidth: true
                text: qsTr("Selecting a provider moves it from its current proxy to this one.")
                font.capitalization: Font.MixedCase
                wrapMode: Text.Wrap
            }
            Repeater {
                id: providers
                model: QA.App.providerStatus
                onItemAdded: providersSection.columnChanged()
                onItemRemoved: providersSection.columnChanged()
                delegate: QA.SettingRow {
                    id: providerRow
                    objectName: "proxyProvider/" + model.itemId.id
                    required property var model
                    required property int index
                    enabled: root.available
                    text: model.name
                    supportText: root.assignedName(model.itemId.id)
                    font.capitalization: Font.MixedCase
                    leader: MD.IconSvg {
                        sourceData: QA.App.providerStatus.svg(providerRow.index)
                        size: 24
                    }
                    trailing: MD.CheckBox {
                        checkState: root.selectedIds.includes(Number(providerRow.model.itemId.id)) ? Qt.Checked : Qt.Unchecked
                        onCheckStateChangeRequested: value => root.selectProvider(providerRow.model.itemId.id, value === Qt.Checked)
                    }
                    onClicked: root.selectProvider(model.itemId.id, !root.selectedIds.includes(Number(model.itemId.id)))
                }
            }
            MD.Label {
                Layout.fillWidth: true
                visible: providers.count === 0
                text: qsTr("Add a provider first, then assign it here.")
                font.capitalization: Font.MixedCase
                wrapMode: Text.Wrap
            }
        }
        MD.Label {
            Layout.fillWidth: true
            visible: text.length > 0
            text: settingsQuery.error || updateQuery.error || (settingsQuery.status === QA.Enum.Finished && !root.available ? qsTr("This proxy no longer exists.") : "")
            color: MD.Token.color.error
            wrapMode: Text.Wrap
        }
        MD.Button {
            visible: settingsQuery.status === QA.Enum.Error || updateQuery.status === QA.Enum.Error
            text: qsTr("Reload")
            onClicked: settingsQuery.reload()
        }
    }
}

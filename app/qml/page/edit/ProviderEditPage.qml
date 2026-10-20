pragma ComponentBehavior: Bound
import QtCore
import QtQuick
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

MD.Page {
    id: root

    required property QA.item_id itemId
    readonly property var model: QA.App.providerStatus.itemById(itemId)
    readonly property var meta: QA.App.providerStatus.metaById(itemId)
    property string pendingTmpProvider: ""
    property bool waitingForSettings: false
    readonly property bool modified: model.name != m_tf_name.text || authInfoModified || m_settings_form.modified || pendingTmpProvider.length > 0
    readonly property bool authInfoModified: root.model.authInfo.serverUrl != m_tf_server.text || (m_auth_loader.item?.modified ?? false)

    font.capitalization: Font.Capitalize
    title: qsTr('Provider Edit')
    bottomPadding: radius
    scrolling: !m_flick.atYBeginning

    actions: [
        MD.Action {
            text: qsTr("Reset")
            icon.name: MD.Token.icon.restart_alt
            enabled: root.modified && !m_update_query.querying
            onTriggered: {
                const it = m_auth_loader.item;
                if (it?.reset)
                    it.reset();
                m_tf_name.text = root.model.name;
                m_tf_server.text = root.model.authInfo.serverUrl;
                m_settings_form.reset();
                root.pendingTmpProvider = "";
                root.waitingForSettings = false;
            }
        },
        MD.Action {
            text: qsTr("Update")
            icon.name: MD.Token.icon.save
            enabled: root.modified && m_settings_query.status === QA.Enum.Finished && !m_update_query.querying
                && !(root.authInfoModified && m_auth_loader.sourceComponent === m_comp_qr && root.pendingTmpProvider.length === 0)
            busy: m_update_query.querying
            onTriggered: root.submitUpdate("")
        }
    ]

    function submitUpdate(tmpProvider) {
        if (tmpProvider) pendingTmpProvider = tmpProvider;
        if (m_settings_query.status !== QA.Enum.Finished) {
            waitingForSettings = pendingTmpProvider.length > 0;
            return;
        }
        if (m_update_query.querying) return;
        waitingForSettings = false;
        const req = m_update_query.req;
        req.tmpProvider = pendingTmpProvider;
        if (root.authInfoModified && !pendingTmpProvider) {
            const info = root.model.authInfo;
            m_auth_loader.item.updateInfo(info);
            info.serverUrl = m_tf_server.text;
            req.authInfo = info;
        } else {
            req.clearAuthInfo();
        }
        req.name = m_tf_name.text;
        req.settingsJson = JSON.stringify(m_settings_form.values);
        req.settingsRevision = m_settings_query.revision;
        m_update_query.req = req;
        m_update_query.reload();
    }
    QA.ProviderSettingsQuery {
        id: m_settings_query
        providerId: root.itemId
        Component.onCompleted: reload()
        onStatusChanged: {
            if (status === QA.Enum.Finished && root.waitingForSettings)
                root.submitUpdate(root.pendingTmpProvider);
        }
    }

    QA.UpdateProviderQuery {
        id: m_update_query
        providerId: root.itemId
        onFieldErrorsChanged: m_settings_form.fieldErrors = fieldErrors
        onStatusChanged: {
            if (status == QA.Enum.Finished)
                root.MD.MProp.page.pop();
        }
    }

    MD.VerticalFlickable {
        id: m_flick
        anchors.fill: parent
        bottomMargin: 24
        contentHeight: m_main.implicitHeight

        ColumnLayout {
            id: m_main
            enabled: !m_update_query.querying
            anchors.centerIn: parent
            height: implicitHeight
            width: Math.min(600, Math.max(300, m_flick.width - 32))
            spacing: 12

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                MD.IconSvg {
                    Layout.alignment: Qt.AlignHCenter
                    sourceData: root.meta.svg
                    size: 42
                }
                MD.Label {
                    typescale: MD.Token.typescale.title_large
                    text: root.model.typeName
                }
            }

            QA.SettingSection {
                Layout.fillWidth: true
                title: qsTr("Basic information")
                spacing: 12

                MD.TextField {
                    id: m_tf_name
                    type: MD.Enum.TextFieldFilled
                    Layout.minimumWidth: 300
                    Layout.fillWidth: true
                    placeholderText: qsTr('Provider Name')
                    text: root.model.name
                }
            }

            QA.SettingSection {
                Layout.fillWidth: true
                title: qsTr("Authentication")
                spacing: 12

                MD.HorizontalListView {
                    id: m_auth_type_view
                    Layout.fillWidth: true
                    implicitHeight: 36
                    visible: count > 1
                    spacing: 8
                    property list<var> models: [
                        {
                            name: qsTr('Username'),
                            comp: m_comp_username,
                            is: function (info) {
                                return info.hasUsername;
                            }
                        },
                        {
                            name: qsTr('Phone'),
                            comp: m_comp_username,
                            is: function (info) {
                                return info.hasPhone;
                            }
                        },
                        {
                            name: qsTr('Email'),
                            comp: m_comp_email,
                            is: function (info) {
                                return info.hasEmail;
                            }
                        },
                        {
                            name: qsTr('Qr'),
                            comp: m_comp_qr,
                            is: function (info) {
                                return info.hasQr;
                            }
                        }
                    ]
                    currentIndex: {
                        const info = root.model.authInfo;
                        for (let i = 0; i < model.length; i++) {
                            if (model[i].is(info))
                                return i;
                        }
                        return 0;
                    }
                    model: root.meta.authTypes.map(el => models[el])
                    delegate: MD.InputChip {
                        required property int index
                        required property var model
                        icon.name: ''
                        selected: m_auth_type_view.currentIndex == index
                        text: model.name
                        onSelectionRequested: m_auth_type_view.currentIndex = index
                    }
                }

                MD.TextField {
                    id: m_tf_server
                    Layout.fillWidth: true
                    visible: root.meta.hasServerUrl
                    type: MD.Enum.TextFieldFilled
                    text: root.model.authInfo.serverUrl
                    placeholderText: qsTr('Server Url')
                }

                Loader {
                    id: m_auth_loader
                    enabled: m_settings_query.status === QA.Enum.Finished && !m_update_query.querying
                    Layout.fillWidth: true
                    sourceComponent: m_auth_type_view.model[m_auth_type_view.currentIndex].comp
                }
            }

            QA.ProviderSettingsForm {
                id: m_settings_form
                Layout.fillWidth: true
                fields: m_settings_query.fields
                originalValues: m_settings_query.values
                enabled: m_settings_query.status === QA.Enum.Finished && !m_update_query.querying
            }
            MD.Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: m_settings_query.error || m_update_query.error
                color: MD.Token.color.error
                wrapMode: Text.Wrap
            }
            MD.Button {
                visible: m_settings_query.status === QA.Enum.Error || m_update_query.status === QA.Enum.Error
                text: m_settings_query.status === QA.Enum.Error ? qsTr("Retry settings") : qsTr("Reload settings")
                onClicked: m_settings_query.reload()
            }
            Component {
                id: m_comp_email
                QA.AuthEmail {
                    anchors.fill: parent
                    Component.onCompleted: {
                        const auth = root.model.authInfo;
                        if (auth.hasEmail) {
                            originEmail = auth.email.emil;
                            originPw = auth.email.pw;
                        }
                    }
                }
            }

            Component {
                id: m_comp_username
                QA.AuthUsername {
                    anchors.fill: parent
                    Component.onCompleted: {
                        const auth = root.model.authInfo;
                        if (auth.hasUsername) {
                            originUsername = auth.username.username;
                            originPw = auth.username.pw;
                        }
                    }
                }
            }
            Component {
                id: m_comp_qr
                QA.AuthQr {
                    QA.CreateTmpProviderQuery {
                        id: m_tmp_provider_query
                        typeName: root.model.typeName
                        networkProviderId: root.itemId
                        Component.onCompleted: reload()
                    }
                    QA.AuthProviderQuery {
                        id: m_auth_query
                        tmpProvider: m_tmp_provider_query.data.key
                        onStatusChanged: {
                            if (status == QA.Enum.Finished) {
                                if (data.code == QM.AuthResult.Ok) {
                                    root.submitUpdate(tmpProvider);
                                }
                            }
                        }
                    }
                    tmpProvider: m_auth_query.tmpProvider
                    query: m_auth_query
                    serverUrl: m_tf_server.text
                    anchors.fill: parent
                }
            }
        }
    }
}

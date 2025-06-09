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
    readonly property bool modified: model.name != m_tf_name.text || authInfoModified
    readonly property bool authInfoModified: root.model.authInfo.serverUrl != m_tf_server.text || (m_auth_loader.item?.modified ?? false)

    font.capitalization: Font.Capitalize
    title: qsTr('provider edit')
    bottomPadding: radius
    scrolling: !m_flick.atYBeginning

    MD.VerticalFlickable {
        id: m_flick
        anchors.fill: parent
        bottomMargin: 24
        contentHeight: m_main.implicitHeight

        ColumnLayout {
            id: m_main
            anchors.centerIn: parent
            height: implicitHeight
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

            MD.TextField {
                id: m_tf_name
                type: MD.Enum.TextFieldFilled
                Layout.minimumWidth: 300
                Layout.fillWidth: true
                placeholderText: qsTr('Provider Name')
                text: root.model.name
            }

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
                    action: MD.Action {
                        icon.name: ''
                        checked: m_auth_type_view.currentIndex == index
                        onTriggered: {
                            m_auth_type_view.currentIndex = index;
                        }
                        text: model.name
                    }
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
                Layout.fillWidth: true
                sourceComponent: m_auth_type_view.model[m_auth_type_view.currentIndex].comp
            }

            RowLayout {
                MD.Button {
                    text: qsTr("reset")
                    onClicked: {
                        const it = m_auth_loader.item;
                        if (it?.reset)
                            it.reset();
                        m_tf_name.text = root.model.name;
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                QA.UpdateProviderQuery {
                    id: m_update_query
                    providerId: root.model.itemId
                    onStatusChanged: {
                        if (status == QA.Enum.Finished)
                            root.MD.MProp.page.pop();
                    }
                }
                QA.ReplaceProviderQuery {
                    id: m_replace_query
                    providerId: root.model.itemId
                    onStatusChanged: {
                        if (status == QA.Enum.Finished)
                            root.MD.MProp.page.pop();
                    }
                }

                MD.Button {
                    enabled: root.modified
                    text: qsTr("update")
                    onClicked: {
                        const query = m_update_query;
                        const req = query.req;

                        if (root.authInfoModified) {
                            const info = root.model.authInfo;
                            m_auth_loader.item.updateInfo(info);
                            info.serverUrl = m_tf_server.text;
                            req.authInfo = info;
                        } else {
                            req.clearAuthInfo();
                        }

                        req.name = m_tf_name.text;
                        query.req = req;
                        query.reload();
                    }
                }
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
                        Component.onCompleted: reload()
                    }
                    QA.AuthProviderQuery {
                        id: m_auth_query
                        tmpProvider: m_tmp_provider_query.data.key
                        onStatusChanged: {
                            if (status == QA.Enum.Finished) {
                                if (data.code == QM.AuthResult.Ok) {
                                    const query = m_replace_query;
                                    const req = query.req;
                                    req.tmpProvider = tmpProvider;
                                    query.req = req;
                                    query.reload();
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

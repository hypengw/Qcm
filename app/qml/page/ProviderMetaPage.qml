pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import Qcm.App as QA
import Qcm.Msg as QM
import Qcm.Material as MD

MD.Page {
    id: root
    required property var meta
    property QM.authInfo authInfo

    QA.CreateTmpProviderQuery {
        id: m_tmp_provider_query
        typeName: root.meta.typeName
        Component.onCompleted: reload()
    }

    QA.AuthProviderQuery {
        id: m_auth_query
        tmpProvider: m_tmp_provider_query.data.key

        onStatusChanged: {
            if (status == QA.Enum.Finished) {
                if (data.code == QM.AuthResult.Ok) {
                    const query = m_add_query;
                    const req = query.req;
                    req.name = m_tf_name.text;
                    req.tmpProvider = tmpProvider;
                    query.req = req;
                    query.reload();
                }
            }
        }
    }

    QA.AddProviderQuery {
        id: m_add_query
        onStatusChanged: {
            if (status == QA.Enum.Finished) {
                QA.Notifier.providerAdded(m_tf_name.text);
                root.MD.MProp.page.pop();
            }
        }
    }

    MD.VerticalFlickable {
        anchors.fill: parent
        bottomMargin: 24
        contentHeight: m_main.implicitHeight

        ColumnLayout {
            id: m_main
            anchors.centerIn: parent
            Layout.preferredWidth: 300
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
                    text: root.meta.typeName
                }
            }

            MD.TextField {
                id: m_tf_name
                type: MD.Enum.TextFieldFilled
                Layout.minimumWidth: 300
                Layout.fillWidth: true
                placeholderText: qsTr('Provider Name')
            }

            MD.Label {
                text: qsTr('auth')
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
                        comp: m_comp_username
                    },
                    {
                        name: qsTr('Phone'),
                        comp: m_comp_username
                    },
                    {
                        name: qsTr('Email'),
                        comp: m_comp_email
                    },
                    {
                        name: qsTr('Qr'),
                        comp: m_comp_qr
                    }
                ]
                currentIndex: 0
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
                placeholderText: qsTr('Server Url')
            }

            Loader {
                id: m_auth_loader
                Layout.fillWidth: true
                sourceComponent: m_auth_type_view.model[m_auth_type_view.currentIndex].comp
            }

            MD.Label {
                Layout.fillWidth: true
                color: MD.MProp.color.error
                visible: text
                text: m_auth_query.failed
            }

            MD.Button {
                Layout.fillWidth: true
                // enabled: qr_login.status !== QA.Enum.Querying
                visible: m_auth_loader.sourceComponent != m_comp_qr
                type: MD.Enum.BtFilled
                font.capitalization: Font.Capitalize
                highlighted: true
                text: qsTr('connect')

                onClicked: {
                    const query = m_auth_query;
                    const req = query.req;
                    const info = root.authInfo;
                    m_auth_loader.item.updateInfo(info);
                    info.serverUrl = m_tf_server.text;
                    req.authInfo = info;
                    query.reload();
                }
            }
        }
        Component {
            id: m_comp_email
            QA.AuthEmail {
                anchors.fill: parent
            }
        }

        Component {
            id: m_comp_username
            QA.AuthUsername {
                anchors.fill: parent
            }
        }
        Component {
            id: m_comp_qr
            QA.AuthQr {
                tmpProvider: m_auth_query.tmpProvider
                query: m_auth_query
                name: m_tf_name.text
                serverUrl: m_tf_server.text
                typeName: root.meta.typeName
                anchors.fill: parent
            }
        }
    }
}

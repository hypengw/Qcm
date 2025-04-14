pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Msg as QM
import Qcm.Material as MD

MD.Page {
    id: root
    required property var meta
    property QM.authInfo authInfo

    QA.AddProviderQuery {
        id: m_add_query
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
                text: m_add_query.failed
            }

            MD.Button {
                Layout.fillWidth: true
                // enabled: qr_login.status !== QA.Enum.Querying
                type: MD.Enum.BtFilled
                font.capitalization: Font.Capitalize
                highlighted: true
                text: qsTr('connect')

                onClicked: {
                    const req = m_add_query.req;
                    req.authInfo = m_auth_loader.item.authInfo();
                    req.name = m_tf_name.text;
                    req.typeName = root.meta.typeName;
                    m_add_query.reload();
                }
            }
        }
        Component {
            id: m_comp_email

            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                MD.TextField {
                    id: m_tf_email
                    Layout.fillWidth: true
                    type: MD.Enum.TextFieldFilled
                    placeholderText: qsTr('Email')
                }
                MD.TextField {
                    id: m_tf_pw
                    Layout.fillWidth: true
                    type: MD.Enum.TextFieldFilled
                    placeholderText: qsTr('Password')
                }

                property QM.emailAuth emailAuth
                function authInfo() {
                    emailAuth.email = m_tf_email.text;
                    emailAuth.pw = m_tf_pw.text;
                    root.authInfo.serverUrl = m_tf_server.text;
                    root.authInfo.email = emailAuth;
                    return root.authInfo;
                }
            }
        }

        Component {
            id: m_comp_username

            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                Connections {
                    function onAuthResultChanged() {
                    }
                    target: root
                }

                MD.TextField {
                    id: m_tf_username
                    Layout.fillWidth: true
                    type: MD.Enum.TextFieldFilled
                    placeholderText: qsTr('User Name')
                }
                MD.TextField {
                    id: m_tf_pw
                    Layout.fillWidth: true
                    type: MD.Enum.TextFieldFilled
                    placeholderText: qsTr('Password')
                }
                property QM.usernameAuth usernameAuth
                function authInfo() {
                    usernameAuth.username = m_tf_username.text;
                    usernameAuth.pw = m_tf_pw.text;
                    root.authInfo.serverUrl = m_tf_server.text;
                    root.authInfo.username = usernameAuth;
                    return root.authInfo;
                }
            }
        }
    }
}

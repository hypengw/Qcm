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
    property QM.usernameAuth usernameAuth

    QA.AddProviderQuery {
        id: m_add_query
    }

    ColumnLayout {
        anchors.centerIn: parent
        Layout.preferredWidth: 300
        spacing: 12

        MD.IconSvg {
            Layout.alignment: Qt.AlignHCenter
            source: 'data:image/svg+xml;utf8,' + root.meta.svg
            size: 96
        }

        MD.TextField {
            id: m_tf_name
            type: MD.Enum.TextFieldFilled
            Layout.fillWidth: true
            placeholderText: qsTr('Provider Name')
        }

        MD.Label {
            text: qsTr('auth')
        }

        Loader {
            id: m_auth_loader
            Layout.fillWidth: true
            sourceComponent: m_comp_username
        }
    }

    Component {
        id: m_comp_username

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            MD.TextField {
                id: m_tf_server
                Layout.minimumWidth: 300
                Layout.fillWidth: true
                type: MD.Enum.TextFieldFilled
                placeholderText: qsTr('Server Url')
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

            MD.Button {
                Layout.fillWidth: true
                // enabled: qr_login.status !== QA.Enum.Querying
                type: MD.Enum.BtFilled
                font.capitalization: Font.Capitalize
                highlighted: true
                text: qsTr('connect')

                Component.onCompleted:
                //tf_username.accepted.connect(clicked);
                //tf_password.accepted.connect(clicked);
                {}
                onClicked: {
                    root.usernameAuth.username = m_tf_username.text;
                    root.usernameAuth.pw = m_tf_pw.text;
                    root.authInfo.serverUrl = m_tf_server.text;
                    root.authInfo.username = root.usernameAuth;
                    const req = m_add_query.req;
                    req.authInfo = root.authInfo;
                    req.name = m_tf_name.text;
                    req.typeName = root.meta.typeName;
                    m_add_query.reload();
                }
            }
        }
    }
}

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
    property int code: m_add_query.data.code

    onCodeChanged: {
        if (code == QM.AuthResult.Ok) {
            QA.Notifier.providerAdded(m_tf_name.text);
            root.MD.MProp.page.pop();
        }
    }

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
                text: m_add_query.failed
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
        Component {
            id: m_comp_qr

            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                QA.QrAuthUrlQuery {
                    id: m_qr_query
                    typeName: root.meta.typeName
                }

                MD.StackView {
                    id: m_qr_stack
                    Layout.fillWidth: true

                    initialItem: comp_qr_wait_scan

                    implicitHeight: currentItem.implicitHeight

                    Connections {
                        function onCodeChanged() {
                            const code = root.code;
                            const stack = m_qr_stack;
                            if (code == QM.AuthResult.QrExpired) {
                                m_qr_query.reload();
                            }
                            switch (code) {
                            case QM.AuthResult.Ok:
                            case QM.AuthResult.QrWaitComform:
                                {
                                    if (stack.depth == 1)
                                        stack.pushItem(comp_qr_wait_comfirm, {}, T.StackView.PushTransition);
                                    break;
                                }
                            default:
                                {
                                    if (stack.depth > 1) {
                                        stack.pop(null);
                                    }
                                }
                            }
                        }
                        target: root
                    }
                    Component.onCompleted: {
                        m_qr_query.reload();
                    }
                }
                Component {
                    id: comp_qr_wait_scan
                    Column {
                        MD.Pane {
                            anchors.horizontalCenter: parent.horizontalCenter
                            backgroundColor: 'white'
                            elevation: MD.Token.elevation.level3
                            padding: 12
                            radius: 12

                            MD.Image {
                                id: qr_image
                                cache: false
                                source: `image://qr/${m_qr_query.data.url}`
                                sourceSize.height: 200
                                sourceSize.width: 200
                            }
                        }
                    }
                }

                Component {
                    id: comp_qr_wait_comfirm
                    ColumnLayout {
                        MD.Image {
                            Layout.alignment: Qt.AlignHCenter
                            source: QA.Util.image_url(m_add_query.data.qrAvatarUrl)
                            sourceSize.height: 96
                            sourceSize.width: 96
                            radius: height / 2
                        }
                        MD.Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: m_add_query.data.qrName
                        }
                        MD.Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: m_add_query.data.message
                        }
                    }
                }
                Timer {
                    interval: 2000
                    repeat: true
                    running: true

                    property QM.qrAuth qrAuth
                    onTriggered: {
                        if (m_add_query.querying)
                            return;

                        qrAuth.key = m_qr_query.data.key;
                        root.authInfo.serverUrl = m_tf_server.text;
                        root.authInfo.qr = qrAuth;
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
}

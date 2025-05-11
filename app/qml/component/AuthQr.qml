pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import Qcm.App as QA
import Qcm.Msg as QM
import Qcm.Material as MD

ColumnLayout {
    id: root
    spacing: 12

    property var query: null
    property string serverUrl
    property string name
    property string typeName
    property string tmpProvider
    property QM.authInfo authInfo

    function updateInfo(info) {
    }

    QA.QrAuthUrlQuery {
        id: m_qr_query
        typeName: root.typeName
        tmpProvider: root.tmpProvider
        onTmpProviderChanged: reload()
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
                source: QA.Util.image_url(root.query.data.qrAvatarUrl)
                sourceSize.height: 96
                sourceSize.width: 96
                radius: height / 2
            }
            MD.Label {
                Layout.alignment: Qt.AlignHCenter
                text: root.query.data.qrName
            }
            MD.Label {
                Layout.alignment: Qt.AlignHCenter
                text: root.query.data.message
            }
        }
    }
    Timer {
        interval: 2000
        repeat: true
        running: true

        property QM.qrAuth auth
        onTriggered: {
            const query = root.query;
            if (query.querying)
                return;
            if (m_qr_query.data.key == 0)
                return;

            const info = root.authInfo;

            auth.key = m_qr_query.data.key;
            info.serverUrl = root.serverUrl;
            info.qr = auth;
            const req = query.req;
            req.authInfo = info;
            req.name = root.name;
            req.typeName = root.typeName;
            root.query.reload();
        }
    }
}

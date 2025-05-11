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
    property string originEmail
    property string originPw
    readonly property bool modified: m_tf_email.text != originEmail || m_tf_pw.text != originPw

    MD.TextField {
        id: m_tf_email
        Layout.fillWidth: true
        type: MD.Enum.TextFieldFilled
        text: root.originEmail
        placeholderText: qsTr('Email')
    }
    MD.TextField {
        id: m_tf_pw
        Layout.fillWidth: true
        type: MD.Enum.TextFieldFilled
        text: root.originPw
        placeholderText: qsTr('Password')
    }

    property QM.emailAuth auth
    function updateInfo(info) {
        auth.email = m_tf_email.text;
        auth.pw = m_tf_pw.text;
        info.email = auth;
    }
    function reset() {
        m_tf_email.text = root.originEmail;
        m_tf_pw.text = root.originPw;
    }
}

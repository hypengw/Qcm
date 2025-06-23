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

    property string originUsername
    property string originPw
    readonly property bool modified: m_tf_username.text != originUsername || m_tf_pw.text != originPw

    MD.TextField {
        id: m_tf_username
        Layout.fillWidth: true
        type: MD.Enum.TextFieldFilled
        text: root.originUsername
        placeholderText: qsTr('User Name')
    }
    MD.TextField {
        id: m_tf_pw
        Layout.fillWidth: true
        type: MD.Enum.TextFieldFilled
        text: root.originPw
        placeholderText: qsTr('Password')
    }
    property QM.usernameAuth auth
    function updateInfo(info) {
        auth.username = m_tf_username.text;
        auth.pw = m_tf_pw.text;
        info.username = auth;
    }

    function reset() {
        m_tf_username.text = root.originUsername;
        m_tf_pw.text = root.originPw;
    }
}

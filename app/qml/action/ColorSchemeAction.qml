import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    icon.name: MD.Token.isDarkTheme ? MD.Token.icon.dark_mode : MD.Token.icon.light_mode
    onTriggered: QA.Global.toggleColorScheme()
}
pragma ComponentBehavior: Bound
import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root

    property int displayMode
    property Item menuParent
    signal selectDisplayMode(int mode)

    icon.name: QA.Util.displayModeIcon(displayMode)
    onTriggered: source => {
        const anchor = (source as MD.MenuItemBase)?.menu?.anchorItem ?? (source as Item) ?? menuParent;
        MD.Util.showPopup(m_display_mode_menu, {
            displayMode: displayMode,
            anchorItem: anchor
        }, anchor);
    }
    Component {
        id: m_display_mode_menu
        QA.DisplayModeMenu {
            modal: true
            dim: false
            onDisplayModeChanged: root.selectDisplayMode(displayMode)
        }
    }
}

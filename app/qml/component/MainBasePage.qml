import QtCore
import QtQuick

import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0

    property int pageIndex: -1

    header: MD.AppBar {
        id: m_bar
        title: root.title
        leadingAction: MD.Action {
            icon.name: root.canBack ? MD.Token.icon.arrow_back : MD.Token.icon.menu
            onTriggered: {
                if (root.canBack)
                    root.back();
                else
                    m_drawer.open();
            }
        }
    }
    title: m_page_stack.currentItem?.title ?? ""
    canBack: m_page_stack.canBack

        //control._canBack ? m_back_action : (Window.window?.barAction ?? null)
        //MD.Action {
        //    id: m_back_action
        //    icon.name: MD.Token.icon.arrow_back
        //    onTriggered: {
        //        if (control.canBack) {
        //            control.back();
        //        } else if (control.pageContext?.canBack) {
        //            control.pageContext.back();
        //        }
        //    }
        //}

}

import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    showBackground: true
    MD.MProp.backgroundColor: MD.MProp.color.surface

    Item {
        anchors.fill: parent
        implicitWidth: m_stack.implicitWidth
        implicitHeight: m_stack.implicitHeight
        QA.PageStack {
            id: m_stack
            anchors.fill: parent

            MD.MProp.page: m_page_ctx
            MD.PageContext {
                id: m_page_ctx
                inherit: root.MD.MProp.page
                showHeader: true
                leadingAction: MD.Action {
                    icon.name: MD.Token.icon.arrow_back
                    onTriggered: {
                        m_stack.back();
                    }
                }

                onPushItem: function (comp, props) {
                    m_stack.pushItem(comp, props, T.StackView.PushTransition);
                }
            }

            initialItem: QA.AddProviderPage {
                showHeader: false
            }
        }

        MD.IconButton {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.leftMargin: 4
            anchors.bottomMargin: 4

            action: QA.SettingAction {}
        }
    }
}

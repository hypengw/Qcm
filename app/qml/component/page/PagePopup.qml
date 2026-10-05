import QtQuick
import QtQuick.Window
import QtQuick.Templates as T

import Qcm.Material as MD

MD.Popup {
    id: root
    property bool fillHeight: false
    property bool fillWidth: false
    property var props: ({})
    required property string source

    parent: T.Overlay.overlay
    width: Math.min(400, parent.width)
    height: Math.min(implicitHeight, parent.height * 0.8)

    mdState.textColor: MD.MProp.color.on_surface
    mdState.backgroundColor: MD.MProp.color.surface
    MD.MProp.backgroundColor: MD.MProp.color.surface

    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)

    Binding on height {
        value: root.parent.height
        when: root.fillHeight
    }
    Binding on width {
        value: root.parent.width
        when: root.fillWidth
    }

    radius: MD.MProp.size.isCompact ? 0 : MD.Token.shape.corner.large
    modal: !MD.MProp.size.isCompact

    Binding {
        when: root.MD.MProp.size.isCompact
        root.fillHeight: true
        root.fillWidth: true
        root.padding: 0
        root.verticalPadding: 0
    }

    MD.PageContext {
        id: m_page_context
        showHeader: true
        // headerBackgroundOpacity: 0
        backgroundRadius: root.radius
        radius: root.radius
        leadingAction: MD.Action {
            icon.name: MD.Token.icon.arrow_back
            onTriggered: {
                const cur = m_stack.currentItem;
                if (cur?.canBack) {
                    cur.back();
                } else if (m_stack.depth > 1) {
                    m_stack.popCurrentItem();
                } else {
                    root.close();
                }
            }
        }
    }

    MD.Pool {
        id: m_pool
        onObjectAdded: function (obj, key) {
            const item = obj as Item;
            if (m_stack.pushItem(item)) {
                const attach = item.MD.PageStack;
                attach.removed.connect(m_pool, function () {
                    if (!m_pool.removeObject(obj)) {
                        console.error('remove failed: ', obj);
                    }
                });
                attach.statusChanged.connect(item, function () {
                    if (!attach || !m_stack)
                        return;

                    if (attach.status == MD.PageStack.Active) {
                        m_stack.lastImplicitHeight = 0;
                    } else if (attach.status == MD.PageStack.Deactivating) {
                        m_stack.lastImplicitHeight = Qt.binding(function () {
                            return item.implicitHeight;
                        });
                    }
                });

                m_stack.lastImplicitHeight = m_stack.implicitHeight;
            } else {
                m_pool.removeObject(obj);
            }
        }
    }

    onSourceChanged: {
        m_pool.add(source, props);
    }

    contentItem: MD.PageStack {
        id: m_stack
        property real lastImplicitHeight: 0
        implicitHeight: Math.max(lastImplicitHeight, currentItem?.implicitHeight ?? 0)

        MD.MProp.page: m_page_context
        Connections {
            function onPushItem(comp, props) {
                m_pool.add(comp, props);
            }
            function onPop() {
                if (m_stack.depth > 1)
                    m_stack.popCurrentItem();
            }
            target: m_page_context
        }
    }
    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside
}

import QtQuick
import QtQuick.Window

import Qcm.Material as MD

MD.Popup {
    id: root
    property bool fillHeight: false
    property bool fillWidth: false
    property bool backGestureEnabled: false
    property var props: ({})
    required property string source
    property bool m_complete: false

    parent: MD.Overlay.overlay
    readyForOpen: m_stack.currentItem !== null
    implicitWidth: 400
    width: Math.min(implicitWidth, parent?.width ?? 0)
    height: m_size.size.height

    mdState.textColor: MD.MProp.color.on_surface
    mdState.backgroundColor: MD.MProp.color.surface
    MD.MProp.backgroundColor: MD.MProp.color.surface

    x: Math.round(((parent?.width ?? 0) - width) / 2)
    y: Math.round(((parent?.height ?? 0) - height) / 2)

    Binding on width {
        value: root.parent?.width ?? 0
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
            if (m_stack.pushItem(item, m_stack.depth === 0 ? MD.PageStack.Immediate : MD.PageStack.Animated)) {
                const attach = item.MD.PageStack;
                attach.removed.connect(m_pool, function () {
                    if (!m_pool.removeObject(obj)) {
                        console.error('remove failed: ', obj);
                    }
                });
            } else {
                m_pool.removeObject(obj);
            }
        }
    }

    onAboutToHide: m_stack.abortPreview()

    onSourceChanged: {
        if (m_complete && source.length)
            addPage(source, props);
    }

    Component.onCompleted: {
        m_complete = true;
        if (source.length)
            addPage(source, props);
    }

    function addPage(source, props) {
        m_pool.add(source, Object.assign({}, props, {
            parent: m_page_host
        }));
    }

    contentItem: MD.PageStack {
        id: m_stack
        implicitHeight: destinationSize.height
        backGestureEnabled: root.backGestureEnabled && root.MD.MProp.size.isCompact && depth > 1 && !(currentItem?.canBack ?? false)
        clip: true
        sizeTransition: MD.SizeTransition {
            id: m_size
            targetSize: Qt.size(root.width, root.fillHeight ? (root.parent?.height ?? 0) : root.implicitHeight)
            maximumSize: Qt.size(root.width, (root.parent?.height ?? 0) * (root.fillHeight ? 1 : 0.8))
            enabled: root.opened && !root.fillHeight
        }

        // Provide the page context before component completion.
        Item {
            id: m_page_host
            visible: false
        }

        MD.MProp.page: m_page_context
        Connections {
            function onPushItem(comp, props) {
                root.addPage(comp, props);
            }
            function onPop() {
                if (m_stack.depth > 1)
                    m_stack.popCurrentItem();
            }
            target: m_page_context
        }
    }
    closePolicy: MD.Popup.CloseOnEscape | MD.Popup.CloseOnPressOutside
}

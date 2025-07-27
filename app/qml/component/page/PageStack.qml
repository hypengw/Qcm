import QtQuick
import QtQuick.Templates as T

import Qcm.Material as MD

MD.StackView {
    id: root

    readonly property var pages: new Map()

    property bool canBack: (currentItem?.canBack ?? false) || depth > 1
    function back() {
        if (currentItem?.canBack) {
            currentItem.back();
        } else {
            pop_page();
        }
    }

    implicitHeight: currentItem.implicitHeight
    implicitWidth: currentItem.implicitWidth

    Behavior on implicitHeight {
        NumberAnimation {
            easing.type: Easing.InOutQuad
            duration: 350
        }
    }

    MD.Pool {
        id: m_pool
        onObjectAdded: function (obj, key) {
            const item = root.push(obj) as Item;
            if (item) {
                item.T.StackView.removed.connect(function () {
                    if (!m_pool.removeObject(obj)) {
                        console.error('remove failed: ', obj);
                    }
                });
            }
        }
    }

    function push_page(url, props = {}) {
        const key = JSON.stringify({
            "url": url,
            "props": MD.Util.paramsString(props)
        });
        const item = pages.get(key);
        if (item) {
            popup(item);
        } else {
            m_pool.addWithKey(key, url, props);
        }
    }

    function pop_page(bottom) {
        if (bottom === null) {
            pop(null);
            m_pool.clear();
        } else {
            const item = pop();
            m_pool.removeObject(item);
        }
    }

    function popup(item) {
        const items = [];
        while (currentItem !== item)
            items.unshift(pop(T.StackView.Immediate));
        if (items.length === 0)
            return;
        const target = pop(T.StackView.Immediate);
        push(items, T.StackView.Immediate);
        push(target);
    }
}

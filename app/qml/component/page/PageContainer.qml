import QtQuick
import Qcm.Material as MD

MD.PageStack {
    id: root

    readonly property bool canBack: currentItem?.canBack ?? false

    function back() {
        currentItem.back();
    }

    // Inherit the page context before PageStack starts the enter transition.
    Item {
        id: m_page_host
        visible: false
    }

    MD.Pool {
        id: m_pool
        onObjectAdded: function (obj, key) {
            if (obj === root.currentItem)
                return;
            root.completeTransition();
            const accepted = root.replaceCurrentItem(obj);
            if (!key && accepted) {
                obj.MD.PageStack.removed.connect(m_pool, function () {
                    if (!m_pool.removeObject(obj)) {
                        console.error(`remove failed(${obj}): ${key}`);
                    }
                });
            } else if (!accepted && !key) {
                m_pool.removeObject(obj);
            }
        }
    }

    function switchTo(page_url, props, is_cache = true) {
        if (is_cache) {
            const key = JSON.stringify({
                "url": page_url,
                "props": props
            });
            m_pool.addWithKey(key, page_url, Object.assign({}, props, {parent: m_page_host}));
        } else {
            m_pool.add(page_url, Object.assign({}, props, {parent: m_page_host}));
        }
    }
}

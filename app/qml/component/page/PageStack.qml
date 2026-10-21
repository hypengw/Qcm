import QtQuick
import Qcm.Material as MD

MD.PageStack {
    id: root

    property bool canBack: (currentItem?.canBack ?? false) || depth > 1
    function back() {
        if (currentItem?.canBack) {
            currentItem.back();
        } else {
            pop_page();
        }
    }

    implicitHeight: implicitContentHeight
    implicitWidth: implicitContentWidth

    // Supply the page context before component completion and the enter transition.
    Item {
        id: m_page_host
        visible: false
    }

    MD.Pool {
        id: m_push_pool
        onObjectAdded: (obj, key) => root.acceptPage(obj, m_push_pool, false)
    }
    MD.Pool {
        id: m_replace_pool
        onObjectAdded: (obj, key) => root.acceptPage(obj, m_replace_pool, true)
    }

    function acceptPage(item, pool, replace) {
        if (replace ? replaceCurrentItem(item) : pushItem(item)) {
            item.MD.PageStack.removed.connect(pool, function () {
                pool.removeObject(item);
            });
        } else {
            pool.removeObject(item);
        }
    }

    function push_page(source, props = {}) {
        m_push_pool.add(source, Object.assign({}, props, {
            parent: m_page_host
        }));
    }

    function replace_page(source, props = {}) {
        m_replace_pool.add(source, Object.assign({}, props, {
            parent: m_page_host
        }));
    }

    function pop_page(bottom) {
        if (bottom === null) {
            while (depth > 2) {
                if (!popCurrentItem(MD.PageStack.Immediate))
                    return;
            }
        }
        if (depth > 1)
            popCurrentItem();
    }
}

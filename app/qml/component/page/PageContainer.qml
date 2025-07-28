import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.StackView {
    id: root

    readonly property bool canBack: currentItem?.canBack ?? false

    function back() {
        currentItem.back();
    }

    MD.Pool {
        id: m_pool
        onObjectAdded: function (obj, key) {
            const item = root.replaceCurrentItem(obj) as Item;
            if (!key && item) {
                item.T.StackView.removed.connect(m_pool, function () {
                    if (!m_pool.removeObject(obj)) {
                        console.error(`remove failed(${obj}): ${key}`);
                    }
                });
            }
        }
    }

    function switchTo(page_url, props, is_cache = true) {
        if (is_cache) {
            const key = JSON.stringify({
                "url": page_url,
                "props": props
            });
            m_pool.addWithKey(key, page_url, props);
        } else {
            m_pool.add(page_url, props);
        }
    }
}

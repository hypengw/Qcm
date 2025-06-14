import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.StackView {
    id: root

    readonly property alias current_page: root.m_current_page
    property string m_current_page: ''

    readonly property bool canBack: currentItem?.canBack ?? false

    function back() {
        currentItem.back();
    }

    MD.Pool {
        id: m_pool
        onObjectAdded: function (obj, is_cache) {
            const item = root.replaceCurrentItem(obj);
            if (!is_cache && item) {
                (item as Item).T.StackView.removed.connect(function () {
                    if (!m_pool.removeObject(obj)) {
                        console.error('remove failed: ', obj);
                    }
                });
            }
        }
    }

    function switchTo(page_url, props, is_cache = true) {
        const key = JSON.stringify({
            "url": page_url,
            "props": props
        });
        if (key === m_current_page)
            return;
        m_current_page = key;
        m_pool.addWithKey(key, page_url, props, is_cache);
    }
}

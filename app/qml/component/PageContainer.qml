import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

StackView {
    id: root

    readonly property alias current_page: root.m_current_page
    property string m_current_page: ''
    readonly property var m_page_cache: new Map()

    function switchTo(page_url, props, is_cache = true) {
        const key = JSON.stringify({
            "url": page_url,
            "props": props
        });
        if (key === m_current_page)
            return ;

        if (is_cache) {
            let cache = m_page_cache.get(key);
            if (!cache) {
                cache = QA.create_item(page_url, props, null);
                m_page_cache.set(key, cache);
            }
            replace(currentItem, cache);
        } else {
            replace(currentItem, page_url, props);
        }
        m_current_page = key;
    }

    Component.onDestruction: {
        m_page_cache.forEach((page) => {
            return page.destroy();
        });
    }
}

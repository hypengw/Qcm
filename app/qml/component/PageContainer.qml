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
        if (page_url === m_current_page)
            return ;

        if (is_cache) {
            let cache = m_page_cache.get(page_url);
            if (!cache) {
                cache = QA.create_item(page_url, props, null);
                m_page_cache.set(page_url, cache);
            }
            replace(currentItem, cache);
        } else {
            replace(currentItem, page_url, props);
        }
        m_current_page = page_url;
    }

    Component.onDestruction: {
        m_page_cache.forEach((page) => {
            return page.destroy();
        });
    }
}

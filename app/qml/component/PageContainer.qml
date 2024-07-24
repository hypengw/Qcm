import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

StackView {
    id: root

    readonly property alias current_page: root.m_current_page
    property string m_current_page: ''
    readonly property var m_page_cache: new Map()

    readonly property bool canBack: currentItem?.canBack ?? false

    function back() {
        currentItem.back();
    }

    function switchByKey(key, url_or_comp, props, is_cache) {
        if (is_cache) {
            let cache = m_page_cache.get(key);
            if (!cache) {
                cache = MD.Tool.create_item(url_or_comp, props, null);
                cache.visible = false;
                m_page_cache.set(key, cache);
            }
            replace(currentItem, cache);
        } else {
            replace(currentItem, url_or_comp, props);
        }
        m_current_page = key;
    }
    function switchTo(page_url, props, is_cache = true) {
        const key = JSON.stringify({
                "url": page_url,
                "props": props
            });
        switchToComp;
        if (key === m_current_page)
            return;
        switchByKey(key, page_url, props, is_cache);
    }
    function switchToComp(name, comp, props, is_cache = true) {
        switchByKey(name, comp, props, is_cache);
    }

    Component.onDestruction: {
        m_page_cache.forEach(page => {
                return page.destroy();
            });
    }
}

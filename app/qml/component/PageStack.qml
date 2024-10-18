import QtQuick
import QtQuick.Controls.Basic

import Qcm.Material as MD

StackView {
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

    function push_page(url, props = {}) {
        const key = JSON.stringify({
            "url": url,
            "props": props
        });
        let item = pages.get(key);
        if (item) {
            popup(item);
        } else {
            item = MD.Util.create_item(url, props, null);
            if (item) {
                pages.set(key, item);
                push(item);
            }
        }
    }

    function pop_page(bottom) {
        if (bottom === null) {
            pop(null);
            pages.forEach(v => {
                return v.destroy(1000);
            });
            pages.clear();
        } else {
            const item = pop();
            Array.from(pages.entries()).filter(el => {
                return el[1] === item;
            }).map(el => {
                pages.delete(el[0]);
                item.destroy(1000);
            });
        }
    }

    function popup(item) {
        const items = [];
        while (currentItem !== item)
            items.unshift(pop(StackView.Immediate));
        if (items.length === 0)
            return;
        const target = pop(StackView.Immediate);
        push(items, StackView.Immediate);
        push(target);
    }

    Component.onDestruction: {
        pages.forEach(page => {
            page?.destroy();
        });
    }
}

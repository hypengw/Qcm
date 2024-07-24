import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Service.Ncm as QNcm

StackView {
    id: root

    readonly property var pages: new Map()

    function push_page(url, props) {
        const key = JSON.stringify({
            "url": url,
            "props": props
        });
        let item = pages.get(key);
        if (item) {
            popup(item);
        } else {
            item = MD.Tool.create_item(url, props, null);
            pages.set(key, item);
            push(item);
        }
    }

    function pop_page(bottom) {
        if (bottom === null) {
            pop(null);
            pages.forEach((v) => {
                return v.destroy(1000);
            });
            pages.clear();
        } else {
            const item = pop();
            Array.from(pages.entries()).filter((el) => {
                return el[1] === item;
            }).map((el) => {
                pages.delete(el[0]);
                item.destroy(1000);
            });
        }
    }

    function popup(item) {
        const items = [];
        while (currentItem !== item)items.unshift(pop(StackView.Immediate))
        if (items.length === 0)
            return ;

        const target = pop(StackView.Immediate);
        push(items, StackView.Immediate);
        push(target);
    }

}

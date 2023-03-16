import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QcmApp
import ".."
import "../component"

StackView {
    // fold:
    // split: 1, stack: 1
    // split: 1, stack: 2
    // unfold:
    // split: 2, stack: 1

    id: root

    property bool rightAbove: false
    readonly property bool folded: split.count === 1
    required property int leftMin
    required property int rightMin
    required property Item leftPage
    required property Item rightPage
    property int m_left_fold_width: leftMin

    function add_to_split(item) {
        split.addItem(item);
        // seems stack change this
        item.opacity = 1;
        item.visible = true;
    }

    function add_to_stack(item) {
        root.push(item);
        item.anchors.fill = item.parent;
    }

    function pop_right_from_stack() {
        if (depth === 2) {
            rightPage.anchors.fill = null;
            const rp = pop();
            if (rp)
                rp.StackView.removed.disconnect(add_right_to_split);

            return rp;
        }
        return null;
    }

    // as slot
    function add_right_to_split() {
        add_to_split(rightPage);
        split.visible = true;
    }

    onRightAboveChanged: {
        if (!folded)
            return ;

        if (rightAbove)
            add_to_stack(rightPage);
        else
            pop_right_from_stack();
    }
    onWidthChanged: {
        // do nothing if
        // no item
        // busy
        if (split.count === 0 || busy)
            return ;

        // need a valid left width
        // as split no busy prop, check manunlly
        if (!folded && width >= leftPage.width + rightMin)
            m_left_fold_width = leftPage.width;

        if (folded && width >= m_left_fold_width + rightMin) {
            // split count 1 -> 2
            var rp = pop_right_from_stack();
            // use visible to delay animation
            split.visible = false;
            if (rp) {
                const sig = rp.StackView.removed;
                sig.connect(add_right_to_split);
            } else {
                add_right_to_split();
            }
        } else if (!folded && width > 0 && width < leftPage.width + rightMin) {
            // split count 2 -> 1
            split.visible = false;
            const rp = split.takeItem(1);
            if (rp) {
                if (rightAbove)
                    add_to_stack(rightPage);

            }
            split.visible = !rightAbove;
        }
    }
    Component.onCompleted: {
        add_to_split(leftPage);
        leftPage.SplitView.minimumWidth = leftMin;
        add_to_split(rightPage);
        rightPage.SplitView.minimumWidth = rightMin;
    }

    initialItem: SplitView {
        id: split
    }

}

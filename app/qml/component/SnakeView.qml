pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

ListView {
    id: root

    property int m_snake_id: 0
    property bool bottomToTop: false

    function show(text, duration) {
        model.insert(0, {
            "text": text,
            "duration": duration,
            "id": get_snake_id()
        });
        if (model.count > 1) {
            model.remove(1);
        }
    }

    function get_snake_id() {
        m_snake_id = (m_snake_id + 1) % 1000;
        return m_snake_id;
    }

    function model_remove(snake_id) {
        for (let i = 0; i < model.count; i++) {
            const m = model.get(i);
            if (m.id === snake_id) {
                model.remove(i);
                return;
            }
        }
    }

    z: Infinity
    spacing: 5
    verticalLayoutDirection: bottomToTop ? ListView.BottomToTop : ListView.TopToBottom
    interactive: false

    remove: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                to: 0
                duration: 500
            }
        }
    }

    add: Transition {
        id: addTrans
        ParallelAnimation {
            NumberAnimation {
                properties: "height"
                easing.type: Easing.InOutCubic
                from: 0
                to: addTrans.ViewTransition.item.height
                duration: 200
            }
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 200
            }
        }
    }

    header: Item {
        implicitHeight: 32
    }

    delegate: Item {
        id: dg_bar
        anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
        width: Math.min(Math.max(implicitWidth, 200), ListView.view.width)
        implicitWidth: children[0].implicitWidth
        implicitHeight: children[0].implicitHeight
        clip: true
        required property var model
        function close() {
            model_remove(model.id);
        }

        MD.SnakeBar {
            x: 0
            y: 0
            width: parent.width
            height: implicitHeight

            text: dg_bar.model.text

            onClosed: dg_bar.close()

            Timer {
                running: true
                repeat: false
                interval: dg_bar.model.duration
                onTriggered: dg_bar.close()
            }
        }
    }

    model: ListModel {}
}

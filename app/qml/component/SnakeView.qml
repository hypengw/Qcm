import QtQuick 2.0

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
                return ;
            }
        }
    }

    z: Infinity
    spacing: 5
    verticalLayoutDirection: bottomToTop ? ListView.BottomToTop : ListView.TopToBottom
    interactive: false

    displaced: Transition {
        NumberAnimation {
            properties: 'y'
            easing.type: Easing.InOutQuad
        }

    }

    header: Item {
        implicitHeight: 32
    }

    delegate: SnakeBar {
        anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
        width: Math.min(Math.max(implicitWidth, 200), ListView.view.width)
        text: model.text

        Timer {
            running: true
            repeat: false
            interval: model.duration
            onTriggered: {
                root.model_remove(model.id);
            }
        }

    }

    model: ListModel {
    }

}

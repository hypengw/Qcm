import QtQuick
import QtQuick.Window
import Qcm.App as QA

Flickable {
    id: root

    implicitHeight: m_bar.implicitHeight
    boundsBehavior: Flickable.StopAtBounds

    // manual topMargin
    contentHeight: windowHeight + Math.abs(y)
    readonly property int windowHeight: Window.height
    readonly property real contentYRadio: visibleArea.yPosition / Math.max((1 - visibleArea.heightRatio), 0.001)
    property bool closed: true

    // onFlickStarted: resetToBoundsOnFlick()
    onMovementStarted: {
        state = 'drag';
    }
    onFlickStarted: {
        chooseState();
        cancelFlick();
    }
    onMovementEnded: {
        chooseState();
    }
    function chooseState() {
        if (state != 'drag')
            return;
        let toOpen = contentY > m_content.y / 2;
        if (flicking) {
            if (verticalVelocity > 0) {
                toOpen = true;
            } else if (verticalVelocity < 0) {
                toOpen = false;
            }
        }
        state = toOpen ? 'open' : 'close';
    }

    state: 'close'

    states: [
        State {
            name: 'close'
            PropertyChanges {
                root.contentY: 0
                root.closed: true
                restoreEntryValues: false
            }
        },
        State {
            name: 'drag'
            PropertyChanges {
                root.closed: false
                restoreEntryValues: false
            }
        },
        State {
            name: 'open'
            PropertyChanges {
                root.contentY: m_content.y
                root.closed: false
                restoreEntryValues: false
            }
        }
    ]

    transitions: [
        Transition {
            to: 'close'
            SequentialAnimation {
                NumberAnimation {
                    property: 'contentY'
                    duration: 300
                }
                PropertyAction {
                    property: 'closed'
                }
            }
        },
        Transition {
            to: 'open'
            NumberAnimation {
                property: 'contentY'
                duration: 300
            }
        }
    ]

    Item {
        height: root.contentHeight
        width: parent.width

        Item {
            id: m_content
            y: parent.height - height
            height: root.windowHeight
            width: parent.width
            QA.PlayingPage {
                id: m_page
                anchors.fill: parent
                opacity: root.contentYRadio
            }
            QA.PlayBar {
                id: m_bar
                width: parent.width
                opacity: 1 - root.contentYRadio
            }
        }
    }
}

import QtQuick
import QtQuick.Window
import Qcm.App as QA

Flickable {
    id: root
    implicitHeight: m_bar.implicitHeight
    boundsBehavior: Flickable.StopAtBounds
    contentHeight: Window.height

    onFlickStarted: resetToBoundsOnFlick()
    onMovementEnded: resetToBoundsOnFlick()
    onHeightChanged: resetToBoundsOnResize()

    NumberAnimation {
        id: toOpen
        target: root
        property: 'contentY'
        from: root.contentY
        to: root.contentHeight / 2
        duration: 350
        easing.type: Easing.OutCubic
        running: false
    }

    NumberAnimation {
        id: toClose
        target: root
        property: 'contentY'
        from: root.contentY
        to: 0
        duration: 350
        easing.type: Easing.OutCubic
        running: false
    }

    function resetToBoundsOnFlick() {
        if (!atYBeginning || !atYEnd) {
            if (verticalVelocity > 0) {
                toOpen.restart();
            } else if (verticalVelocity < 0) {
                toClose.restart();
            } else {
                // i.e. when verticalVelocity === 0
                if (contentY > contentHeight / 4) {
                    toOpen.restart();
                } else {
                    toClose.restart();
                }
            }
        }
    }

    function resetToBoundsOnResize() {
        if (contentY > contentHeight / 4) {
            contentY = contentHeight / 2;
        } else {
            contentY = 0;
        }
    }

    Item {
        id: m_content
        height: root.contentHeight
        width: parent.width

        QA.PlayingPage {
            id: m_page
            anchors.fill: parent
        }
        QA.PlayBar {
            id: m_bar
            width: parent.width
        }

        Rectangle {
            anchors.fill: parent
            color: 'red'
            opacity: 0.2
        }
    }
}

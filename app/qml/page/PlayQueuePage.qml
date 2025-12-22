pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0
    title: QA.App.playqueue.name
    bottomPadding: radius
    scrolling: !m_view.atYBeginning

    actions: [
        MD.Action {
            icon.name: MD.Token.icon.delete
            onTriggered: {
                const q = QA.App.playqueue;
                if (q.canRemove) {
                    q.clear();
                } else {
                    QA.Action.toast(`Not support for ${q.name}`);
                }
            }
        }
    ]

    MD.VerticalListView {
        id: m_view
        anchors.fill: parent
        expand: true
        currentIndex: model.currentIndex
        highlightMoveDuration: 1000
        highlightMoveVelocity: -1
        model: QA.App.playqueue
        topMargin: 8
        bottomMargin: 8
        cacheBuffer: 32

        MD.FontMetrics {
            id: item_font_metrics
            typescale: MD.Token.typescale.body_medium
            readonly property real minimumWidth: item_font_metrics.advanceWidth(m_view.count.toString())
        }

        footer: Item {}

        delegate: DropArea {
            id: m_delegate
            required property int index
            required property var model
            readonly property bool is_playing: ListView.isCurrentItem
            width: ListView.view.width
            implicitHeight: m_item.implicitHeight

            onEntered: drag => {
                QA.App.playqueue.move(drag.source.model.index, model.index);
            }

            MD.ListItem {
                id: m_item
                index: m_delegate.index
                model: m_delegate.model
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                width: m_delegate.width
                height: implicitHeight

                rightPadding: 4
                onClicked: {
                    const m = m_delegate.ListView.view.model;
                    if (m.canJump) {
                        QA.Action.play(model.itemId);
                    } else {
                        QA.Action.toast(qsTr(`Not support for ${m.name}`));
                    }
                }

                Drag.active: dragArea.drag.active
                Drag.source: m_delegate
                Drag.hotSpot.x: dragArea.width / 2
                Drag.hotSpot.y: dragArea.height / 2
                states: [
                    State {
                        when: m_item.Drag.active
                        ParentChange {
                            target: m_item
                            parent: m_view
                        }
                        AnchorChanges {
                            target: m_item
                            anchors.verticalCenter: undefined
                            anchors.horizontalCenter: undefined
                        }
                    }
                ]

                contentItem: RowLayout {
                    spacing: 12

                    MD.Icon {
                        implicitWidth: 40
                        implicitHeight: 40
                        name: MD.Token.icon.drag_indicator

                        MouseArea {
                            id: dragArea
                            anchors.fill: parent
                            drag.target: m_item
                            drag.axis: Drag.YAxis
                            drag.threshold: 0
                            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                        }
                    }

                    QA.ListenIcon {
                        Layout.fillWidth: false
                        Layout.minimumWidth: item_font_metrics.minimumWidth + 2
                        index: model.index
                        isPlaying: m_delegate.is_playing
                    }

                    MD.Text {
                        id: item_idx
                        Layout.fillWidth: true
                        typescale: MD.Token.typescale.body_medium
                        verticalAlignment: Qt.AlignVCenter
                        text: model.name
                    }

                    MD.IconButton {
                        icon.name: MD.Token.icon.remove

                        onClicked: {
                            const q = QA.App.playqueue;
                            if (q.canRemove) {
                                m_view.model.removeRow(model.index);
                            } else {
                                QA.Action.toast(`Not support for ${q.name}`);
                            }
                        }
                    }
                }
            }
        }
    }
}

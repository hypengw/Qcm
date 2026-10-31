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
    readonly property QA.DynamicIdQueue radioQueue: QA.App.playqueue.radioQueue

    actions: [
        MD.Action {
            text: root.radioQueue?.restartLabel || qsTr("Restart radio")
            icon.name: MD.Token.icon.refresh
            visible: root.radioQueue?.canRestart ?? false
            enabled: visible && !root.radioQueue?.configuring
            onTriggered: root.radioQueue.restart()
        },
        MD.Action {
            text: qsTr("Clear queue")
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

    MD.VerticalLazyList {
        id: m_view
        anchors.fill: parent
        expand: true
        currentIndex: source.model.currentIndex
        source: QA.ItemModelSource {
            model: QA.App.playqueue
            roles: ["itemId", "name"]
        }
        extraTopMargin: 8
        extraBottomMargin: 8
        cacheExtent: 32

        header: Flow {
            width: m_view.width
            padding: m_configurations.count > 0 ? 8 : 0
            spacing: 8

            Repeater {
                id: m_configurations
                model: root.radioQueue?.configuration ?? []

                QA.ComboChip {
                    required property var modelData
                    width: Math.min(implicitWidth, parent.width - parent.leftPadding - parent.rightPadding)
                    enabled: !root.radioQueue?.configuring
                    options: modelData.options
                    value: modelData.value
                    onValueChangeRequested: value => root.radioQueue.configure(modelData.key, value)
                }
            }
        }

        MD.FontMetrics {
            id: item_font_metrics
            typescale: MD.Token.typescale.body_medium
            readonly property real minimumWidth: item_font_metrics.advanceWidth(m_view.count.toString())
        }

        footer: Item {}

        delegate: DropArea {
            id: m_delegate
            required property MD.LazyRow row
            readonly property int index: row.index
            readonly property var model: row.value
            readonly property bool is_playing: m_view.currentIndex === index

            implicitHeight: m_item.implicitHeight

            onEntered: drag => {
                QA.App.playqueue.move(drag.source.row.index, row.index);
            }

            MD.ListItem {
                id: m_item
                itemRow: m_delegate.row
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                width: m_delegate.width
                height: implicitHeight

                rightPadding: 4
                onClicked: {
                    const m = m_view.source.model;
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
                        index: m_delegate.row.index
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
                                m_view.source.model.removeRow(m_delegate.row.index);
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

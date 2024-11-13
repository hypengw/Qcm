import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as QC
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0
    title: QA.App.playqueue.name //qsTr('Queue')
    bottomPadding: radius
    scrolling: !m_view.atYBeginning

    actions: [
        QC.Action {
            icon.name: MD.Token.icon.delete
            onTriggered: {
                QA.App.playqueue.clear();
            }
        }
    ]

    MD.ListView {
        id: m_view
        anchors.fill: parent
        expand: true
        currentIndex: model.currentIndex
        highlightMoveDuration: 1000
        highlightMoveVelocity: -1
        model: QA.App.playqueue
        topMargin: 8
        bottomMargin: 8

        MD.FontMetrics {
            id: item_font_metrics
            typescale: MD.Token.typescale.body_medium
            readonly property real minimumWidth: item_font_metrics.advanceWidth(m_view.count.toString())
        }

        footer: Item {}

        delegate: MD.ListItem {
            required property var model
            required property var index
            readonly property bool is_playing: ListView.isCurrentItem

            rightPadding: 4
            width: ListView.view.width
            onClicked: {
                const m = ListView.view.model;
                if (m.canJump) {
                    QA.Action.play_by_id(model.itemId);
                } else {
                    QA.Action.toast(qsTr(`not support for ${m.name}`));
                }
            }

            contentItem: RowLayout {
                spacing: 12

                QA.ListenIcon {
                    Layout.fillWidth: false
                    Layout.minimumWidth: item_font_metrics.minimumWidth + 2
                    index: model.index
                    isPlaying: is_playing
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
                            QA.Action.toast(`not support for ${q.name}`);
                        }
                    }
                }
            }
        }
    }
}

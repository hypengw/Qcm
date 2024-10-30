import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0
    title: 'Playlist'
    bottomPadding: radius
    scrolling: !m_view.atYBeginning

    MD.ListView {
        id: m_view
        anchors.fill: parent
        expand: true
        bottomMargin: 8
        currentIndex: model.currentIndex
        highlightMoveDuration: 1000
        highlightMoveVelocity: -1
        model: QA.App.playlist
        reuseItems: true
        topMargin: 8

        MD.FontMetrics {
            id: item_font_metrics
            typescale: MD.Token.typescale.body_medium
            readonly property real minimumWidth: item_font_metrics.advanceWidth(m_view.count.toString())
        }

        footer: Item {}

        delegate: MD.ListItem {
            width: ListView.view.width
            readonly property bool is_playing: ListView.isCurrentItem
            onClicked: {
                QA.Action.play_by_id(model.itemId);
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
                        m_view.model.removeRow(model.index);
                    }
                }
            }
        }
    }
}

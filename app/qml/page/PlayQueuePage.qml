import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0
    title: 'Playlist'

    MD.ListView {
        id: view_playlist
        anchors.fill: parent
        expand: true
        bottomMargin: 8
        currentIndex: model.curIndex
        highlightMoveDuration: 1000
        highlightMoveVelocity: -1
        model: QA.App.playlist
        reuseItems: true
        topMargin: 8

        MD.FontMetrics {
            id: item_font_metrics
            typescale: MD.Token.typescale.body_medium
            readonly property real minimumWidth: item_font_metrics.advanceWidth(view_playlist.count.toString())
        }

        footer: Item {}

        delegate: MD.ListItem {
            width: ListView.view.width
            readonly property bool is_playing: model.song.itemId === QA.App.playlist.cur.itemId
            onClicked: {
                QA.App.playlist.switchTo(model.song);
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
                    text: model.song.name
                }

                MD.IconButton {
                    icon.name: MD.Token.icon.remove

                    onClicked: {
                        QA.App.playlist.remove(model.song.itemId);
                    }
                }
            }
        }
    }
}

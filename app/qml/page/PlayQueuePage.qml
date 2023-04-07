import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."
import "../component"
import "../part"

Page {
    id: root
    padding: 0
    title: 'Playlist'

    MListView {
        id: view_playlist
        anchors.fill: parent
        bottomMargin: 8
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        currentIndex: model.curIndex
        highlightMoveDuration: 1000
        highlightMoveVelocity: -1
        implicitHeight: contentHeight + 2 * topMargin
        model: QA.playlist
        reuseItems: true
        topMargin: 8

        ScrollBar.vertical: ScrollBar {
        }
        delegate: MItemDelegate {
            width: ListView.view.width

            // highlighted: model.song.itemId === QA.playlist.cur.itemId
            onClicked: {
                QA.playlist.switchTo(model.song);
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 12

                Label {
                    Layout.minimumWidth: Theme.font.w_unit * view_playlist.count.toString().length + 2
                    horizontalAlignment: Qt.AlignRight
                    opacity: 0.6
                    text: index + 1
                }
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: model.song.name
                }
                MRoundButton {
                    flat: true
                    icon.name: Theme.ic.remove

                    onClicked: {
                        QA.playlist.remove(model.song.itemId);
                    }
                }
            }
        }
    }
}

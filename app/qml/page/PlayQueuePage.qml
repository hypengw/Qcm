import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0
    title: 'Playlist'

    QA.MListView {
        id: view_playlist
        anchors.fill: parent
        bottomMargin: 8
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        currentIndex: model.curIndex
        highlightMoveDuration: 1000
        highlightMoveVelocity: -1
        implicitHeight: contentHeight + 2 * topMargin
        model: QA.Global.playlist
        reuseItems: true
        topMargin: 8

        ScrollBar.vertical: ScrollBar {
        }
        delegate: MD.ListItem {
            width: ListView.view.width

            // highlighted: model.song.itemId === QA.playlist.cur.itemId
            onClicked: {
                QA.Global.playlist.switchTo(model.song);
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 12

                MD.Text {
                    Layout.minimumWidth: item_idx.typescale.size * view_playlist.count.toString().length
                    horizontalAlignment: Qt.AlignRight
                    typescale: MD.Token.typescale.body_medium
                    opacity: 0.6
                    text: index + 1
                }
                MD.Text {
                    id: item_idx
                    Layout.fillWidth: true
                    typescale: MD.Token.typescale.body_medium
                    elide: Text.ElideRight
                    text: model.song.name
                }
                MD.IconButton {
                    icon.name: MD.Token.icon.remove

                    onClicked: {
                        QA.Global.playlist.remove(model.song.itemId);
                    }
                }
            }
        }
    }
}

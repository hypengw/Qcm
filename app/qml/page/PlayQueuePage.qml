import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0
    title: 'Playlist'
    implicitHeight: view_playlist.contentHeight

    MD.ListView {
        id: view_playlist
        anchors.fill: parent
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

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 12

                StackLayout {
                    Layout.fillHeight: false
                    Layout.fillWidth: false
                    Layout.minimumWidth: item_font_metrics.minimumWidth + 2
                    currentIndex: 0

                    Binding on currentIndex  {
                        value: 1
                        when: is_playing
                    }

                    MD.Text {
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                        typescale: MD.Token.typescale.body_medium
                        opacity: 0.6
                        text: index + 1
                    }
                    MD.Icon {
                        name: MD.Token.icon.equalizer
                        size: 24
                        MD.MatProp.textColor: MD.Token.color.primary
                        horizontalAlignment: Qt.AlignHCenter
                    }
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

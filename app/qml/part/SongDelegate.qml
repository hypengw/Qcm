import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qcm.Material as MD

import ".."
import "../component"

MD.ListItem {
    id: root

    required property int count
    readonly property int count_len: this.count.toString().length
    required property int index
    readonly property bool is_playing: QA.playlist.cur.itemId === modelData.itemId
    required property var modelData
    property string subtitle: ''

    enabled: modelData.canPlay
    highlighted: is_playing
    heightMode: MD.Enum.ListItemTwoLine

    contentItem: RowLayout {
        spacing: 16

        StackLayout {
            Layout.fillHeight: false
            Layout.fillWidth: false
            Layout.minimumWidth: Theme.font.w_unit * root.count_len + 2
            currentIndex: 0

            Binding on currentIndex  {
                value: 1
                when: root.is_playing
            }

            MD.Text {
                horizontalAlignment: Qt.AlignRight
                typescale: MD.Token.typescale.body_medium
                opacity: 0.6
                text: index + 1
            }
            MD.Icon {
                name: Theme.ic.equalizer
                size: 24
                MD.MatProp.textColor: MD.Token.color.primary
                horizontalAlignment: Qt.AlignRight
            }
        }
        ColumnLayout {
            spacing: 0
            MD.Text {
                Layout.fillWidth: true
                text: root.modelData.name
                typescale: MD.Token.typescale.body_large
            }
            RowLayout {
                Repeater {
                    model: root.modelData.tags

                    delegate: ColumnLayout {
                        SongTag {
                            pointSize: Theme.font.small(subtitle_label.font)
                            tag: modelData
                        }
                    }
                }
                MD.Text {
                    id: subtitle_label
                    Layout.fillWidth: true
                    typescale: MD.Token.typescale.body_medium
                    color: MD.MatProp.supportTextColor
                    text: root.subtitle ? root.subtitle : `${QA.join_name(root.modelData.artists, '/')} - ${root.modelData.album.name}`
                }
            }
        }
        MD.Text {
            typescale: MD.Token.typescale.body_medium
            text: Qt.formatDateTime(root.modelData.duration, 'mm:ss')
        }
        RowLayout {
            spacing: 0

            MD.IconButton {
                checked: QA.user_song_set.contains(root.modelData.itemId)
                icon.name: checked ? Theme.ic.favorite : Theme.ic.favorite_border

                onClicked: {
                    QA.querier_user_song.like_song(root.modelData.itemId, !checked);
                }
            }
            MD.IconButton {
                icon.name: Theme.ic.more_vert

                onClicked: {
                    QA.show_popup('qrc:/Qcm/App/qml/part/SongMenu.qml', {
                            "song": modelData,
                            "y": height
                        }, this);
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Material as MD


MD.ListItem {
    id: root

    required property int count
    readonly property int count_len: this.count.toString().length
    required property int index
    readonly property bool is_playing: QA.Global.playlist.cur.itemId === modelData.itemId
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
            Layout.minimumWidth: QA.Theme.font.w_unit * root.count_len + 2
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
                name: MD.Token.icon.equalizer
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
                            tag: modelData
                        }
                    }
                }
                MD.Text {
                    id: subtitle_label
                    Layout.fillWidth: true
                    typescale: MD.Token.typescale.body_medium
                    color: MD.MatProp.supportTextColor
                    text: root.subtitle ? root.subtitle : `${QA.Global.join_name(root.modelData.artists, '/')} - ${root.modelData.album.name}`
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
                checked: QA.Global.user_song_set.contains(root.modelData.itemId)
                icon.name: checked ? MD.Token.icon.favorite : MD.Token.icon.favorite_border

                onClicked: {
                    QA.Global.querier_user_song.like_song(root.modelData.itemId, !checked);
                }
            }
            MD.IconButton {
                icon.name: MD.Token.icon.more_vert

                onClicked: {
                    QA.Global.show_popup('qrc:/Qcm/App/qml/part/SongMenu.qml', {
                            "song": modelData,
                            "y": height
                        }, this);
                }
            }
        }
    }
}

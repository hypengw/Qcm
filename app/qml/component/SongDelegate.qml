import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.ListItem {
    id: root

    property int count: ListView.view?.count ?? 0
    property int index_: index ? index : (model ? model.index : 0)
    readonly property bool is_playing: QA.Global.playlist.cur.itemId === model_.itemId
    property QA.t_song model_: modelData
    property string subtitle: ''

    enabled: model_.canPlay
    highlighted: is_playing
    heightMode: MD.Enum.ListItemTwoLine

    contentItem: RowLayout {
        spacing: 16

        StackLayout {
            Layout.fillHeight: false
            Layout.fillWidth: false
            Layout.minimumWidth: item_font_metrics.advanceWidth(root.count.toString()) + 2
            currentIndex: 0

            Binding on currentIndex  {
                value: 1
                when: root.is_playing
            }

            MD.FontMetrics {
                id: item_font_metrics
                typescale: MD.Token.typescale.body_medium
            }

            MD.Text {
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                typescale: MD.Token.typescale.body_medium
                opacity: 0.6
                text: index_ + 1
            }
            MD.Icon {
                name: MD.Token.icon.equalizer
                size: 24
                MD.MatProp.textColor: MD.Token.color.primary
                horizontalAlignment: Qt.AlignHCenter
            }
        }
        ColumnLayout {
            spacing: 0
            MD.Text {
                Layout.fillWidth: true
                text: root.model_.name
                typescale: MD.Token.typescale.body_large
                verticalAlignment: Qt.AlignVCenter
            }
            RowLayout {
                Repeater {
                    model: root.model_.tags

                    delegate: ColumnLayout {
                        SongTag {
                            tag: modelData
                        }
                    }
                }
                MD.Text {
                    id: subtitle_label
                    Layout.fillWidth: true
                    verticalAlignment: Qt.AlignVCenter
                    typescale: MD.Token.typescale.body_medium
                    color: MD.MatProp.supportTextColor
                    text: root.subtitle ? root.subtitle : `${QA.Global.join_name(root.model_.artists, '/')} - ${root.model_.album.name}`
                }
            }
        }
        MD.Text {
            typescale: MD.Token.typescale.body_medium
            text: Qt.formatDateTime(root.model_.duration, 'mm:ss')
            verticalAlignment: Qt.AlignVCenter
        }
        RowLayout {
            spacing: 0

            MD.IconButton {
                checked: QA.Global.user_song_set.contains(root.model_.itemId)
                icon.name: checked ? MD.Token.icon.favorite : MD.Token.icon.favorite_border

                onClicked: {
                    QA.Global.querier_user_song.like_song(root.model_.itemId, !checked);
                }
            }
            MD.IconButton {
                icon.name: MD.Token.icon.more_vert

                onClicked: {
                    QA.Global.show_popup('qrc:/Qcm/App/qml/component/SongMenu.qml', {
                            "song": model_,
                            "y": height
                        }, this);
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."
import "../component"

MItemDelegate {
    id: root

    required property int count
    readonly property int count_len: this.count.toString().length
    required property int index
    readonly property bool is_playing: QA.playlist.cur.itemId === modelData.itemId
    required property var modelData
    property string subtitle: ''

    enabled: modelData.canPlay
    highlighted: is_playing

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

            Label {
                horizontalAlignment: Qt.AlignRight
                opacity: 0.6
                text: index + 1
            }
            Label {
                color: Theme.color.tertiary
                font.family: Theme.font.icon_round.family
                font.pointSize: 18
                horizontalAlignment: Qt.AlignRight
                text: Theme.ic.equalizer
            }
        }
        ColumnLayout {
            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: root.modelData.name
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
                Label {
                    id: subtitle_label
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font.pointSize: Theme.ts.label_small.size
                    opacity: 0.6
                    text: root.subtitle ? root.subtitle : `${QA.join_name(root.modelData.artists, '/')} - ${root.modelData.album.name}`
                }
            }
        }
        Label {
            text: Qt.formatDateTime(root.modelData.duration, 'mm:ss')
        }
        RowLayout {
            spacing: 0

            MRoundButton {
                readonly property bool liked: QA.user_song_set.contains(root.modelData.itemId)

                Material.accent: Theme.color.tertiary
                flat: true
                font.family: Theme.font.icon_round.family
                font.pointSize: 12
                highlighted: liked
                text: liked ? Theme.ic.favorite : Theme.ic.favorite_border

                onClicked: {
                    QA.querier_user_song.like_song(root.modelData.itemId, !liked);
                }
            }
            MRoundButton {
                id: btn_menu
                flat: true
                font.family: Theme.font.icon_round.family
                font.pointSize: 12
                text: Theme.ic.more_vert

                onClicked: {
                    const item = comp_menu.createObject(btn_menu);
                    item.closed.connect(() => {
                            item.destroy(1000);
                        });
                    item.open();
                }

                Component {
                    id: comp_menu
                    MMenu {
                        dim: false
                        modal: true
                        y: btn_menu.height

                        Action {
                            icon.name: Theme.ic.play_arrow
                            text: qsTr('Play next')

                            onTriggered: {
                                QA.playlist.appendNext(modelData);
                            }
                        }
                        Action {
                            icon.name: Theme.ic.album
                            text: qsTr('Show album')

                            onTriggered: {
                                QA.route(modelData.album.itemId);
                            }
                        }
                        Action {
                            icon.name: Theme.ic.person
                            text: qsTr('Show artist')

                            onTriggered: {
                                const artists = modelData.artists;
                                if (artists.length === 1)
                                    QA.route(artists[0].itemId);
                                else
                                    QA.show_popup('qrc:/QcmApp/qml/part/ArtistsPopup.qml', {
                                            "model": artists
                                        });
                            }
                        }
                    }
                }
            }
        }
    }
}

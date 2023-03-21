import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."
import "../component"

ItemDelegate {
    id: root

    required property int index
    required property int count
    required property var modelData
    readonly property int count_len: this.count.toString().length
    property string subtitle: ''

    enabled: modelData.canPlay

    contentItem: RowLayout {
        spacing: 16

        StackLayout {
            Layout.fillWidth: false
            Layout.fillHeight: false
            Layout.minimumWidth: Theme.font.w_unit * root.count_len + 2
            currentIndex: 0

            Label {
                horizontalAlignment: Qt.AlignRight
                text: index + 1
                opacity: 0.6
            }

            Label {
                horizontalAlignment: Qt.AlignRight
                font.family: Theme.font.icon_round.family
                font.pointSize: 16
                color: Theme.color.tertiary
                text: Theme.ic.equalizer
            }

            Binding on currentIndex {
                when: QA.playlist.cur.itemId === modelData.itemId
                value: 1
            }

        }

        ColumnLayout {
            Label {
                Layout.fillWidth: true
                text: root.modelData.name
                elide: Text.ElideRight
            }

            RowLayout {
                Repeater {
                    model: root.modelData.tags

                    delegate: ColumnLayout {
                        SongTag {
                            tag: modelData
                            pointSize: Theme.font.small(subtitle_label.font)
                        }

                    }

                }

                Label {
                    id: subtitle_label

                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font.pointSize: Theme.font.small(Theme.font.label_font)
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
                highlighted: liked
                text: liked ? Theme.ic.favorite : Theme.ic.favorite_border
                font.family: Theme.font.icon_round.family
                font.pointSize: 14
                flat: true
                onClicked: {
                    QA.querier_user_song.like_song(root.modelData.itemId, !liked);
                }
            }

            MRoundButton {
                id: btn_menu

                text: Theme.ic.more_vert
                font.family: Theme.font.icon_round.family
                font.pointSize: 14
                flat: true
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
                        y: btn_menu.height
                        modal: true
                        dim: false

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

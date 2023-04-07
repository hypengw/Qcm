import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QcmApp
import ".."
import "../component"
import "../part"
import "../js/util.mjs" as Util

MPage {
    id: root
    header: RowLayout {
        MRoundButton {
            Layout.alignment: Qt.AlignLeft
            flat: true

            action: Action {
                icon.name: Theme.ic.arrow_back

                onTriggered: {
                    QA.sig_route_special('main');
                }
            }
        }
    }

    MPage {
        anchors.fill: parent

        ColumnLayout {
            anchors.centerIn: parent
            width: parent.width

            MPane {
                id: pane_cover

                readonly property int img_size: 240

                Layout.alignment: Qt.AlignHCenter
                Layout.preferredHeight: Layout.preferredWidth
                Layout.preferredWidth: img_size + 2 * padding
                Material.background: Theme.color.surface_2
                Material.elevation: 4
                padding: 4

                Image {
                    height: width
                    source: `image://ncm/${QA.cur_song.album.picUrl}`
                    sourceSize.height: pane_cover.img_size
                    sourceSize.width: pane_cover.img_size
                }
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 12
                Layout.fillWidth: true
                Layout.topMargin: 12
                elide: Text.ElideRight
                font.bold: true
                font.pointSize: Theme.ts.label_large.size
                horizontalAlignment: Text.AlignHCenter
                text: QA.cur_song.name
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.pointSize: Theme.ts.label_medium.size
                horizontalAlignment: Text.AlignHCenter
                text: QA.cur_song.album.name
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.pointSize: Theme.ts.label_medium.size
                horizontalAlignment: Text.AlignHCenter
                text: QA.join_name(QA.cur_song.artists, '/')
            }
            RowLayout {
                Layout.alignment: Qt.AlignHCenter

                MRoundButton {
                    readonly property bool liked: QA.user_song_set.contains(QA.cur_song.itemId)

                    Material.accent: Theme.color.tertiary
                    enabled: QA.cur_song.itemId.valid()
                    flat: true
                    highlighted: liked
                    icon.name: liked ? Theme.ic.favorite : Theme.ic.favorite_border

                    onClicked: {
                        QA.querier_user_song.like_song(QA.cur_song.itemId, !liked);
                    }
                }
                MRoundButton {
                    enabled: QA.playlist.canPrev
                    flat: true
                    icon.name: Theme.ic.skip_previous

                    onClicked: QA.playlist.prev()
                }
                MRoundButton {
                    highlighted: true
                    icon.name: QA.player.playing ? Theme.ic.pause : Theme.ic.play_arrow

                    onClicked: {
                        const player = QA.player;
                        if (player.playing)
                            player.pause();
                        else
                            player.play();
                    }
                }
                MRoundButton {
                    enabled: QA.playlist.canNext
                    flat: true
                    icon.name: Theme.ic.skip_next

                    onClicked: QA.playlist.next()
                }
                MRoundButton {
                    flat: true
                    icon.name: QA.loop_icon

                    onClicked: QA.playlist.iterLoopMode()
                }
            }
            RowLayout {
                Layout.alignment: Qt.AlignHCenter

                Label {
                    readonly property date position: new Date(QA.player.duration * slider.position)

                    opacity: QA.player.duration > 0 ? 1 : 0
                    text: `${Qt.formatDateTime(position, 'mm:ss')}`
                }
                PlaySlider {
                    id: slider
                    Layout.preferredWidth: 220
                }
                Label {
                    readonly property date duration: new Date(QA.player.duration)

                    opacity: QA.player.duration > 0 ? 1 : 0
                    text: `${Qt.formatDateTime(duration, 'mm:ss')}`
                }
            }
            Item {
                Layout.fillWidth: true
            }
        }
    }
}

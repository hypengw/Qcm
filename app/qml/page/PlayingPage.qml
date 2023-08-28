import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qcm.App
import ".."
import "../component"
import "../part"
import "../js/util.mjs" as Util

MPage {
    id: root
    header: Pane {
        clip: false
        implicitHeight: 0

        RowLayout {
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
    }

    ApiContainer {
        SongLyricQuerier {
            id: querier_lyric

            readonly property string combined_lrc: {
                return data.lrc + data.transLrc;
            }

            autoReload: songId.valid()
            songId: QA.cur_song.itemId
        }
    }
    MPage {
        anchors.fill: parent

        RowLayout {
            anchors.fill: parent

            Pane {
                id: play_pane
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 12
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

                            Material.accent: Theme.color.secondary
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
                            opacity: QA.player.duration > 0 ? 1 : 0
                            text: `${Qt.formatDateTime(QA.player.duration_date, 'mm:ss')}`
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                }
            }
            Pane {
                id: lyric_pane
                Layout.fillHeight: true
                Layout.fillWidth: true
                implicitWidth: play_pane.implicitWidth

                ColumnLayout {
                    anchors.fill: parent

                    LrcLyric {
                        id: lrc
                        position: QA.player.position
                        source: querier_lyric.combined_lrc

                        onCurrentIndexChanged: {
                            lyric_view.posTo(currentIndex < 0 ? 0 : currentIndex);
                        }
                    }
                    Pane {
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        MListView {
                            id: lyric_view
                            function posTo(idx) {
                                if (visible) {
                                    anim_scroll.from = contentY;
                                    positionViewAtIndex(idx, ListView.Center);
                                    anim_scroll.to = contentY;
                                    contentY = anim_scroll.from;
                                    anim_scroll.running = !moving;
                                }
                            // seem when not visible, this is not accurate
                            // use a timer to sync
                            }
                            function timer_restart() {
                                if (visible)
                                    timer_scroll.restart();
                            }

                            anchors.fill: parent
                            boundsBehavior: Flickable.StopAtBounds
                            boundsMovement: Flickable.StopAtBounds
                            highlightFollowsCurrentItem: false
                            model: lrc
                            reuseItems: true
                            spacing: 4

                            ScrollBar.vertical: ScrollBar {
                                visible: false
                            }
                            SmoothedAnimation on contentY  {
                                id: anim_scroll

                                property bool manual_stopped: false

                                function manual_stop() {
                                    manual_stopped = true;
                                    stop();
                                }

                                duration: 1000
                                velocity: 200

                                onStopped: {
                                    if (manual_stopped) {
                                        manual_stopped = false;
                                        return;
                                    }
                                    if (lyric_view.count === 0)
                                        return;
                                    const cur = lyric_view.itemAtIndex(lrc.currentIndex);
                                    if (cur) {
                                        const center = cur.y + cur.height / 2;
                                        const list_center = lyric_view.contentY + lyric_view.height / 2;
                                        const diff = Math.abs(center - list_center);
                                        if (diff > 10) {
                                            timer_scroll.triggered();
                                        }
                                    } else {
                                        lyric_view.timer_restart();
                                    }
                                }
                            }
                            delegate: MItemDelegate {
                                Material.foreground: lrc.currentIndex === index ? Theme.color.tertiary : root.Material.foreground
                                width: ListView.view.width

                                contentItem: ColumnLayout {
                                    Label {
                                        Layout.fillWidth: true
                                        font.pointSize: Theme.ts.title_medium.size
                                        horizontalAlignment: Text.AlignHCenter
                                        text: model.content
                                        wrapMode: Text.Wrap
                                    }
                                }

                                onClicked: {
                                    QA.player.position = model.milliseconds;
                                }
                            }
                            footer: Item {
                                height: ListView.view.height / 2
                            }
                            header: Item {
                                height: ListView.view.height / 2
                            }

                            onHeightChanged: timer_restart()
                            onMovingChanged: {
                                if (moving)
                                    anim_scroll.manual_stop();
                            }
                            onVisibleChanged: timer_restart()
                            onWheelMoved: anim_scroll.manual_stop()
                            onWidthChanged: timer_restart()

                            Timer {
                                id: timer_scroll
                                interval: 400
                                repeat: false
                                running: true

                                onTriggered: lrc.currentIndexChanged()
                            }
                        }
                    }
                }
            }
        }
    }
}

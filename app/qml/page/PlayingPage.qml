import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD
import "../js/util.mjs" as Util

MD.Page {
    id: root
    header: MD.Pane {
        clip: false
        implicitHeight: 0

        RowLayout {
            MD.IconButton {
                Layout.alignment: Qt.AlignLeft
                action: Action {
                    icon.name: MD.Token.icon.arrow_back

                    onTriggered: {
                        QA.Global.sig_route_special('main');
                    }
                }
            }
        }
    }

    QA.SongLyricQuerier {
        id: querier_lyric

        readonly property string combined_lrc: {
            return data.lrc + data.transLrc;
        }

        autoReload: songId.valid()
        songId: QA.Global.cur_song.itemId
    }

    MD.Page {
        anchors.fill: parent

        RowLayout {
            anchors.fill: parent

            MD.Pane {
                id: play_pane
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 12
                    width: parent.width

                    MD.Image {
                        Layout.alignment: Qt.AlignHCenter
                        MD.MatProp.elevation: MD.Token.elevation.level2
                        source: `image://ncm/${QA.Global.cur_song.album.picUrl}`
                        sourceSize.height: 240
                        sourceSize.width: 240
                        radius: 16
                    }
                    MD.Text {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.bottomMargin: 12
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        font.bold: true
                        typescale: MD.Token.typescale.title_medium
                        horizontalAlignment: Text.AlignHCenter
                        text: QA.Global.cur_song.name
                    }
                    MD.Text {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.fillWidth: true
                        typescale: MD.Token.typescale.body_medium
                        horizontalAlignment: Text.AlignHCenter
                        text: QA.Global.cur_song.album.name
                    }
                    MD.Text {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.fillWidth: true
                        typescale: MD.Token.typescale.body_medium
                        horizontalAlignment: Text.AlignHCenter
                        text: QA.Global.join_name(QA.Global.cur_song.artists, '/')
                    }
                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter

                        MD.IconButton {
                            readonly property bool liked: QA.Global.user_song_set.contains(QA.Global.cur_song.itemId)

                            enabled: QA.Global.cur_song.itemId.valid()
                            icon.name: liked ? MD.Token.icon.favorite : MD.Token.icon.favorite_border

                            onClicked: {
                                QA.Global.querier_user_song.like_song(QA.Global.cur_song.itemId, !liked);
                            }
                        }
                        MD.IconButton {
                            enabled: QA.Global.playlist.canPrev
                            icon.name: MD.Token.icon.skip_previous

                            onClicked: QA.Global.playlist.prev()
                        }
                        MD.IconButton {
                            icon.name: QA.Global.player.playing ? MD.Token.icon.pause : MD.Token.icon.play_arrow

                            onClicked: {
                                const player = QA.Global.player;
                                if (player.playing)
                                    player.pause();
                                else
                                    player.play();
                            }
                        }
                        MD.IconButton {
                            enabled: QA.Global.playlist.canNext
                            icon.name: MD.Token.icon.skip_next

                            onClicked: QA.Global.playlist.next()
                        }
                        MD.IconButton {
                            flat: true
                            icon.name: QA.Global.loop_icon

                            onClicked: QA.Global.playlist.iterLoopMode()
                        }
                    }
                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter

                        Label {
                            readonly property date position: new Date(QA.Global.player.duration * slider.position)

                            opacity: QA.Global.player.duration > 0 ? 1 : 0
                            text: `${Qt.formatDateTime(position, 'mm:ss')}`
                        }
                        QA.PlaySlider {
                            id: slider
                            Layout.preferredWidth: 220
                        }
                        Label {
                            opacity: QA.Global.player.duration > 0 ? 1 : 0
                            text: `${Qt.formatDateTime(QA.Global.player.duration_date, 'mm:ss')}`
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                }
            }
            MD.Pane {
                id: lyric_pane
                Layout.fillHeight: true
                Layout.fillWidth: true
                implicitWidth: play_pane.implicitWidth

                ColumnLayout {
                    anchors.fill: parent

                    QA.LrcLyric {
                        id: lrc
                        position: QA.Global.player.position
                        source: querier_lyric.combined_lrc

                        onCurrentIndexChanged: {
                            lyric_view.posTo(currentIndex < 0 ? 0 : currentIndex);
                        }
                    }
                    MD.Pane {
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        MD.ListView {
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
                            highlightFollowsCurrentItem: false
                            model: lrc
                            reuseItems: true
                            spacing: 4

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
                            delegate: MD.ListItem {
                                width: ListView.view.width

                                contentItem: ColumnLayout {
                                    property bool current: lrc.currentIndex === index
                                    MD.Text {
                                        Layout.fillWidth: true
                                        typescale: MD.Token.typescale.title_medium
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        text: model.content
                                        color: parent.current ? MD.Token.color.primary : MD.Token.color.on_surface
                                        font.weight: parent.current ? Font.Bold : typescale.weight
                                        maximumLineCount: -1
                                    }
                                }

                                onClicked: {
                                    QA.Global.player.position = model.milliseconds;
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

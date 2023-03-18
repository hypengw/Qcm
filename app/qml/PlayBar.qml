import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QcmApp
import "./component"
import "./part"

Pane {
    Material.elevation: 4
    padding: 0

    ColumnLayout {
        anchors.fill: parent

        Slider {
            id: slider

            readonly property double playing_pos: QA.player.duration > 0 ? QA.player.position / QA.player.duration : 0

            live: false
            Layout.fillWidth: true
            background.implicitHeight: 8
            padding: 0
            to: 1
            onPressedChanged: {
                if (!pressed) {
                    slider_timer.stop();
                    slider_timer.triggered();
                }
            }
            onPositionChanged: {
                if (pressed)
                    slider_timer.recordPos(position);

            }
            onPlaying_posChanged: {
                if (!pressed)
                    value = playing_pos;

            }

            Timer {
                id: slider_timer

                property double pos

                function recordPos(pos_) {
                    pos = pos_;
                    restart();
                }

                interval: 500
                onTriggered: {
                    if (pos > 0) {
                        QA.player.setPos(pos);
                        pos = -1;
                    }
                }
            }

        }

        RowLayout {
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: 8

            Image {
                Layout.preferredWidth: sourceSize.width
                Layout.preferredHeight: sourceSize.height
                sourceSize.width: 48
                sourceSize.height: 48
                source: `image://ncm/${QA.cur_song.album.picUrl}`
            }

            ColumnLayout {
                Label {
                    Layout.fillWidth: true
                    text: QA.cur_song.name
                    elide: Text.ElideRight
                }

                RowLayout {
                    Repeater {
                        model: QA.cur_song.tags

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
                        text: QA.join_name(QA.cur_song.artists, '/')
                    }

                }

            }

            RoundButton {
                font.family: Theme.font.icon_round.family
                font.pointSize: 16
                background.implicitWidth: 32
                enabled: QA.playlist.canPrev
                flat: true
                padding: 8
                text: Theme.ic.skip_previous
                onClicked: QA.playlist.prev()
            }

            MRoundButton {
                highlighted: true
                font.family: Theme.font.icon_round.family
                font.pointSize: 20
                text: {
                    if (QA.player.playing)
                        return Theme.ic.pause;
                    else
                        return Theme.ic.play_arrow;
                }
                onClicked: {
                    const player = QA.player;
                    if (player.playing)
                        player.pause();
                    else
                        player.play();
                }
            }

            RoundButton {
                font.family: Theme.font.icon_round.family
                font.pointSize: 16
                background.implicitWidth: 32
                enabled: QA.playlist.canNext
                flat: true
                padding: 8
                text: Theme.ic.skip_next
                onClicked: QA.playlist.next()
            }

            RoundButton {
                font.family: Theme.font.icon_round.family
                font.pointSize: 16
                background.implicitWidth: 32
                flat: true
                padding: 8
                text: QA.loop_icon
                onClicked: {
                    let mode = QA.playlist.loopMode;
                    switch (mode) {
                    case Playlist.NoneLoop:
                        mode = Playlist.SingleLoop;
                        break;
                    case Playlist.SingleLoop:
                        mode = Playlist.ListLoop;
                        break;
                    case Playlist.ListLoop:
                        mode = Playlist.ShuffleLoop;
                        break;
                    case Playlist.ShuffleLoop:
                        mode = Playlist.NoneLoop;
                        break;
                    }
                    QA.playlist.loopMode = mode;
                }
            }

            RoundButton {
                font.family: Theme.font.icon_round.family
                font.pointSize: 16
                background.implicitWidth: 32
                flat: true
                padding: 8
                text: Theme.ic.queue_music
                onClicked: {
                    pop_playlist.open();
                }

                MPopup {
                    id: pop_playlist

                    title: 'Playlist'

                    Pane {
                        Layout.fillWidth: true
                        Layout.fillHeight: implicitHeight > pop_playlist.leftHeight
                        Layout.alignment: Qt.AlignTop
                        Layout.topMargin: 12
                        Material.background: Theme.color.surface_1
                        Material.elevation: 1
                        padding: 0

                        ListView {
                            id: view_playlist

                            anchors.fill: parent
                            topMargin: 8
                            bottomMargin: 8
                            implicitHeight: contentHeight + 2 * topMargin
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds
                            highlightMoveDuration: 1000
                            highlightMoveVelocity: -1
                            currentIndex: model.curIndex
                            model: QA.playlist

                            delegate: MItemDelegate {
                                width: ListView.view.width
                                // highlighted: model.song.itemId === QA.playlist.cur.itemId
                                onClicked: {
                                    QA.playlist.switchTo(model.song);
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12

                                    Label {
                                        Layout.fillWidth: true
                                        text: model.song.name
                                    }

                                    RoundButton {
                                        font.family: Theme.font.icon_round.family
                                        font.pointSize: 16
                                        flat: true
                                        padding: 8
                                        text: Theme.ic.remove
                                        onClicked: {
                                            QA.playlist.remove(model.song.itemId);
                                        }
                                    }

                                }

                            }

                            ScrollBar.vertical: ScrollBar {
                            }

                        }

                    }

                    Item {
                        Layout.fillHeight: true
                    }

                }

            }

            Label {
                readonly property date duration: new Date(QA.player.duration)
                readonly property date position: new Date(QA.player.duration * slider.position)

                text: `${Qt.formatDateTime(position,
                                           'mm:ss')} / ${Qt.formatDateTime(
                          duration, 'mm:ss')}`
            }

        }

    }

}

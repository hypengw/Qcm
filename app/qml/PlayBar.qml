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

        PlaySlider {
            id: slider
            Layout.fillWidth: true
        }
        RowLayout {
            Layout.bottomMargin: 8
            Layout.leftMargin: 12
            Layout.rightMargin: 12

            Image {
                readonly property string picUrl: QA.cur_song.album.picUrl

                Layout.preferredHeight: sourceSize.height
                Layout.preferredWidth: sourceSize.width
                source: `image://ncm/${picUrl}`
                sourceSize.height: 48
                sourceSize.width: 48

                onStatusChanged: {
                    if (status == Image.Ready)
                        QA.song_cover = App.getImageCache(picUrl, sourceSize);
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true

                    onClicked: {
                        QA.sig_route_special('playing');
                    }
                }
            }
            ColumnLayout {
                Layout.leftMargin: 4

                Label {
                    Layout.fillWidth: true
                    Layout.maximumWidth: implicitWidth + 10
                    Material.foreground: ma_name.containsMouse ? Theme.color.primary : Theme.color.on_background
                    elide: Text.ElideRight
                    text: QA.cur_song.name

                    MouseArea {
                        id: ma_name
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true

                        onClicked: {
                            QA.route(QA.cur_song.album.itemId);
                        }
                    }
                }
                RowLayout {
                    Repeater {
                        model: QA.cur_song.tags

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
                        Layout.maximumWidth: implicitWidth + 10
                        Material.foreground: ma_subtitle.containsMouse ? Theme.color.primary : Theme.color.on_background
                        elide: Text.ElideRight
                        font.pointSize: Theme.ts.label_small.size
                        opacity: 0.6
                        text: QA.join_name(QA.cur_song.artists, '/')

                        MouseArea {
                            id: ma_subtitle
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            hoverEnabled: true

                            onClicked: {
                                const artists = QA.cur_song.artists;
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
            Item {
                Layout.fillWidth: true
            }
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

                onClicked: {
                    QA.playlist.iterLoopMode();
                }
            }
            MRoundButton {
                flat: true
                icon.name: Theme.ic.playlist_play

                onClicked: {
                    pop_playlist.open();
                }

                MPopup {
                    id: pop_playlist
                    title: 'Playlist'
                    width: 400

                    Pane {
                        Layout.alignment: Qt.AlignTop
                        Layout.fillHeight: implicitHeight > pop_playlist.contentMaxHeight
                        Layout.fillWidth: true
                        padding: 0

                        MListView {
                            id: view_playlist
                            anchors.fill: parent
                            bottomMargin: 8
                            boundsBehavior: Flickable.StopAtBounds
                            clip: true
                            currentIndex: model.curIndex
                            highlightMoveDuration: 1000
                            highlightMoveVelocity: -1
                            implicitHeight: contentHeight + 2 * topMargin
                            model: QA.playlist
                            reuseItems: true
                            topMargin: 8

                            ScrollBar.vertical: ScrollBar {
                            }
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
                                    spacing: 12

                                    Label {
                                        Layout.minimumWidth: Theme.font.w_unit * view_playlist.count.toString().length + 2
                                        horizontalAlignment: Qt.AlignRight
                                        opacity: 0.6
                                        text: index + 1
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                        text: model.song.name
                                    }
                                    MRoundButton {
                                        flat: true
                                        icon.name: Theme.ic.remove

                                        onClicked: {
                                            QA.playlist.remove(model.song.itemId);
                                        }
                                    }
                                }
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

                text: `${Qt.formatDateTime(position, 'mm:ss')} / ${Qt.formatDateTime(duration, 'mm:ss')}`
            }
        }
    }
}

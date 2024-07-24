import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Pane {
    id: root

    MD.MatProp.backgroundColor: QA.Global.main_win.smallLayout ? MD.MatProp.color.surface_container : MD.MatProp.color.surface
    MD.MatProp.elevation: QA.Global.main_win.smallLayout ? MD.Token.elevation.level2 : MD.Token.elevation.level0

    readonly property bool isSmall: root.width > 0 && root.width < 650
    padding: 0

    function swich_play() {
        const player = QA.Global.player;
        if (player.playing)
            player.pause();
        else
            player.play();
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Shortcut {
            sequences: ["Space"]
            onActivated: root.swich_play()
        }

        Item {
            Layout.fillWidth: true
            implicitHeight: 4
            visible: !isSmall
            clip: false
            QA.PlaySlider {
                id: slider
                anchors.centerIn: parent
                width: parent.width
                z: 1
            }
        }
        RowLayout {
            Layout.bottomMargin: 8
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.topMargin: 8

            MD.Image {
                readonly property string picUrl: QA.Global.cur_song.album.picUrl
                source: `image://ncm/${picUrl}`
                sourceSize.height: 48
                sourceSize.width: 48
                radius: 8

                onStatusChanged: {
                    if (status == Image.Ready)
                        QA.Global.song_cover = QA.App.getImageCache(picUrl, sourceSize);
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true

                    onClicked: {
                        QA.Global.sig_route_special('playing');
                    }
                }
            }
            ColumnLayout {
                Layout.leftMargin: 4
                spacing: 0

                MD.Text {
                    Layout.fillWidth: true
                    Layout.maximumWidth: implicitWidth + 10
                    MD.MatProp.textColor: ma_name.containsMouse ? MD.Token.color.primary : MD.Token.color.on_background
                    verticalAlignment: Qt.AlignVCenter
                    typescale: MD.Token.typescale.body_medium

                    text: QA.Global.cur_song.name
                    MouseArea {
                        id: ma_name
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true

                        onClicked: {
                            QA.Global.route(QA.Global.cur_song.album.itemId);
                        }
                    }
                }
                RowLayout {
                    Repeater {
                        model: QA.Global.cur_song.tags

                        delegate: ColumnLayout {
                            QA.SongTag {
                                tag: modelData
                            }
                        }
                    }
                    MD.Text {
                        id: subtitle_label
                        Layout.fillWidth: true
                        Layout.maximumWidth: implicitWidth + 10
                        MD.MatProp.textColor: ma_subtitle.containsMouse ? MD.Token.color.primary : MD.Token.color.on_background
                        typescale: MD.Token.typescale.body_medium
                        opacity: 0.6
                        text: QA.Global.join_name(QA.Global.cur_song.artists, '/')
                        verticalAlignment: Qt.AlignVCenter

                        MouseArea {
                            id: ma_subtitle
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            hoverEnabled: true

                            onClicked: {
                                const artists = QA.Global.cur_song.artists;
                                if (artists.length === 1)
                                    QA.Global.route(artists[0].itemId);
                                else
                                    QA.Global.show_page_popup('qrc:/Qcm/App/qml/component/ArtistsPopup.qml', {
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
            Component {
                id: comp_ctl
                RowLayout {
                    MD.IconButton {
                        checked: QA.Global.user_song_set.contains(QA.Global.cur_song.itemId)
                        enabled: QA.Global.cur_song.itemId.valid()
                        icon.name: checked ? MD.Token.icon.favorite : MD.Token.icon.favorite_border

                        onClicked: {
                            QA.Global.querier_user_song.like_song(QA.Global.cur_song.itemId, !checked);
                        }
                    }
                    MD.IconButton {
                        enabled: QA.Global.playlist.canPrev
                        icon.name: MD.Token.icon.skip_previous

                        onClicked: QA.Global.playlist.prev()
                    }
                    MD.IconButton {
                        type: MD.Enum.IBtFilled
                        icon.name: QA.Global.player.playing ? MD.Token.icon.pause : MD.Token.icon.play_arrow

                        onClicked: root.swich_play()
                    }
                    MD.IconButton {
                        enabled: QA.Global.playlist.canNext
                        icon.name: MD.Token.icon.skip_next

                        onClicked: QA.Global.playlist.next()
                    }
                    MD.IconButton {
                        icon.name: QA.Global.loop_icon

                        onClicked: {
                            QA.Global.playlist.iterLoopMode();
                        }
                    }
                    MD.IconButton {
                        icon.name: MD.Token.icon.playlist_play

                        onClicked: {
                            pop_playlist.open();
                        }
                    }
                    QA.VolumeButton {
                        volume: QA.Global.player.volume
                        onVolumeSeted: volume_ => {
                            QA.Global.player.volume = volume_;
                        }
                    }
                    MD.IconButton {
                        icon.name: MD.Token.icon.more_vert

                        onClicked: {
                            const popup = MD.Tool.show_popup('qrc:/Qcm/App/qml/menu/SongMenu.qml', {
                                "song": QA.Global.cur_song,
                                "y": 0
                            }, this);
                            popup.y = -popup.height;
                        }
                    }
                    MD.Text {
                        verticalAlignment: Qt.AlignVCenter
                        typescale: MD.Token.typescale.body_medium
                        readonly property date position: new Date(QA.Global.player.duration * slider.position)
                        text: `${Qt.formatDateTime(position, 'mm:ss')} / ${Qt.formatDateTime(QA.Global.player.duration_date, 'mm:ss')}`
                    }
                }
            }
            Component {
                id: comp_ctl_small
                RowLayout {
                    MD.IconButton {
                        icon.name: QA.Global.player.playing ? MD.Token.icon.pause : MD.Token.icon.play_arrow

                        onClicked: root.swich_play()

                        MD.CircleProgress {
                            anchors.centerIn: parent
                            progress: (QA.Global.player.position / QA.Global.player.duration)
                        }
                    }
                    MD.IconButton {
                        icon.name: MD.Token.icon.playlist_play

                        onClicked: {
                            pop_playlist.open();
                        }
                    }
                }
            }
            StackView {
                id: ctl_stack
                function switch_to() {
                    replace(currentItem, root.isSmall ? comp_ctl_small : comp_ctl);
                }

                Layout.fillHeight: true

                Binding on implicitWidth {
                    value: ctl_stack.currentItem.implicitWidth
                    when: ctl_stack.currentItem
                }
                initialItem: Item {}
                replaceEnter: Transition {
                    LineAnimation {
                        from: 0.5 * ctl_stack.height
                        property: 'y'
                        to: 0
                    }
                    FadeIn {}
                }
                replaceExit: Transition {
                    FadeOut {}
                }

                Component.onCompleted: {
                    root.isSmallChanged.connect(switch_to);
                    root.isSmallChanged();
                }

                QA.PagePopup {
                    id: pop_playlist
                    source: 'qrc:/Qcm/App/qml/page/PlayQueuePage.qml'
                }
            }
        }
    }

    component FadeIn: LineAnimation {
        from: 0.0
        property: "opacity"
        to: 1.0
    }
    component FadeOut: LineAnimation {
        from: 1.0
        property: "opacity"
        to: 0.0
    }
    component LineAnimation: NumberAnimation {
        duration: 200
        easing.type: Easing.OutCubic
    }
}

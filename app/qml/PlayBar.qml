import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Pane {
    id: root

    readonly property var currentSong: QA.App.playlist.currentSong

    // backgroundColor: Window.window?.windowClass === MD.Enum.WindowClassCompact ? MD.MatProp.color.surface_container : MD.MatProp.color.surface
    backgroundColor: MD.Token.color.surface_container
    elevation: Window.window?.windowClass === MD.Enum.WindowClassCompact ? MD.Token.elevation.level2 : MD.Token.elevation.level0

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
            visible: !root.isSmall
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
                readonly property string picUrl: {
                    const cover = root.currentSong.coverUrl;
                    return cover ? cover : root.currentSong.album.picUrl;
                }
                source: QA.Util.image_url(picUrl)
                sourceSize.height: 48
                sourceSize.width: 48
                radius: 8

                onStatusChanged: {
                    if (status == Image.Ready) {
                        const p = root.currentSong.itemId.provider;
                        QA.Global.song_cover = QA.Util.image_cache_of(p, picUrl, sourceSize);
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true

                    onClicked: {
                        QA.Action.route_special('playing');
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

                    text: root.currentSong.name
                    MouseArea {
                        id: ma_name
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true

                        onClicked: {
                            QA.Global.route(root.currentSong.album.itemId);
                        }
                    }
                }
                RowLayout {
                    Repeater {
                        model: root.currentSong.tags

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
                        text: QA.Global.join_name(root.currentSong.artists, '/')
                        verticalAlignment: Qt.AlignVCenter

                        MouseArea {
                            id: ma_subtitle
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            hoverEnabled: true

                            onClicked: {
                                const artists = root.currentSong.artists;
                                if (artists.length === 1)
                                    QA.Global.route(artists[0].itemId);
                                else
                                    QA.Action.popup_page('qrc:/Qcm/App/qml/component/ArtistsPopup.qml', {
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
                        checked: QA.Global.session.user.collection.contains(root.currentSong.itemId)
                        enabled: root.currentSong.itemId.valid()
                        icon.name: checked ? MD.Token.icon.favorite : MD.Token.icon.favorite_border

                        onClicked: {
                            QA.Action.collect(root.currentSong.itemId, !checked);
                        }
                    }
                    MD.IconButton {
                        enabled: QA.App.playlist.canPrev
                        icon.name: MD.Token.icon.skip_previous

                        onClicked: QA.App.playlist.prev()
                    }
                    MD.IconButton {
                        type: MD.Enum.IBtFilled
                        icon.name: QA.Global.player.playing ? MD.Token.icon.pause : MD.Token.icon.play_arrow

                        onClicked: root.swich_play()
                    }
                    MD.IconButton {
                        enabled: QA.App.playlist.canNext
                        icon.name: MD.Token.icon.skip_next

                        onClicked: QA.App.playlist.next()
                    }
                    MD.IconButton {
                        icon.name: QA.Global.loop_icon

                        onClicked: {
                            QA.App.playlist.iterLoopMode();
                        }
                    }
                    MD.IconButton {
                        icon.name: MD.Token.icon.playlist_play

                        onClicked: {
                            QA.Action.popup_special(QA.enums.SRQueue);
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
                            const popup = MD.Util.show_popup(comp_song_menu, {
                                "song": root.currentSong,
                                "y": 0
                            }, this);
                            popup.y = -popup.height;
                        }

                        Component {
                            id: comp_song_menu
                            QA.SongMenu {}
                        }
                    }
                    RowLayout {
                        spacing: 0
                        MD.FontMetrics {
                            id: fm_time
                            typescale: MD.Token.typescale.body_medium
                        }
                        MD.Text {
                            Layout.preferredWidth: fm_time.rect("00:00").width + 2
                            horizontalAlignment: Qt.AlignHCenter
                            verticalAlignment: Qt.AlignVCenter
                            typescale: MD.Token.typescale.body_medium
                            readonly property date position: new Date(QA.Global.player.duration * slider.position)
                            text: `${Qt.formatDateTime(position, 'mm:ss')}`
                        }
                        MD.Text {
                            verticalAlignment: Qt.AlignVCenter
                            typescale: MD.Token.typescale.body_medium
                            text: '/'
                        }
                        MD.Text {
                            Layout.preferredWidth: fm_time.rect("00:00").width + 2
                            horizontalAlignment: Qt.AlignHCenter
                            verticalAlignment: Qt.AlignVCenter
                            typescale: MD.Token.typescale.body_medium
                            text: `${Qt.formatDateTime(QA.Global.player.durationDate, 'mm:ss')}`
                        }
                    }
                }
            }
            Component {
                id: comp_ctl_small
                RowLayout {
                    MD.IconButton {
                        icon.name: QA.Global.player.playing ? MD.Token.icon.pause : MD.Token.icon.play_arrow

                        onClicked: root.swich_play()

                        MD.CircleProgressBar {
                            anchors.centerIn: parent
                            value: (QA.Global.player.position / QA.Global.player.duration)
                        }
                    }
                    MD.IconButton {
                        icon.name: MD.Token.icon.playlist_play

                        onClicked: {
                            QA.Action.popup_special(QA.enums.SRQueue);
                        }
                    }
                }
            }
            MD.StackView {
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

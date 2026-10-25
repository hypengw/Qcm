pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Pane {
    id: root

    readonly property var currentSong: QA.App.playqueue.currentSong
    readonly property QA.item_id currentSongId: QA.App.playqueue.currentSongId
    readonly property QA.EntityView entity: QA.Store.item(root.currentSongId)
    readonly property QA.item_id currentAlbumId: root.entity?.albumId ?? QA.Util.createItemid()
    readonly property list<var> artists: entity?.artists ?? []

    backgroundColor: MD.Token.color.surface_container
    elevation: MD.MProp.size.isCompact ? MD.Token.elevation.level2 : MD.Token.elevation.level0

    readonly property bool isSmall: root.width > 0 && root.width < 650
    padding: 0

    Item {
        anchors.fill: parent
        implicitHeight: m_content.implicitHeight
        implicitWidth: m_content.implicitWidth
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            hoverEnabled: true

            onClicked: {
                QA.Action.togglePlaybar();
            }
        }

        ColumnLayout {
            id: m_content
            anchors.fill: parent
            spacing: 0

            Shortcut {
                sequences: ["Space"]
                onActivated: QA.Action.toggle()
            }

            Item {
                Layout.fillWidth: true
                implicitHeight: 4
                visible: !root.isSmall
                clip: false
                QA.PlaySlider {
                    id: slider
                    handle: null
                    implicitHeight: 20
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

                QA.Image {
                    source: QA.Util.image_url(root.currentSongId)
                    displaySize: Qt.size(48, 48)
                    radius: 8
                }
                ColumnLayout {
                    Layout.leftMargin: 4
                    spacing: 0

                    MD.Text {
                        Layout.fillWidth: true
                        Layout.maximumWidth: implicitWidth + 10
                        MD.MProp.textColor: ma_name.containsMouse ? MD.Token.color.primary : MD.Token.color.on_background
                        verticalAlignment: Qt.AlignVCenter
                        typescale: MD.Token.typescale.body_medium
                        maximumLineCount: 1

                        text: root.currentSong.name
                        MouseArea {
                            id: ma_name
                            anchors.fill: parent
                            enabled: root.currentAlbumId.valid
                            cursorShape: Qt.PointingHandCursor
                            hoverEnabled: true

                            onClicked: {
                                QA.Action.routeItem(root.currentAlbumId);
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
                        MD.ActionLabel {
                            id: subtitle_label
                            Layout.fillWidth: true
                            Layout.maximumWidth: implicitWidth + 10
                            opacity: 0.6
                            verticalAlignment: Qt.AlignVCenter
                            maximumLineCount: 1
                            action: MD.Action {
                                text: QA.Util.joinName(root.artists, '/')
                                onTriggered: {
                                    m_go_to_artist_act.trigger();
                                }
                            }

                            QA.GoToArtistAction {
                                id: m_go_to_artist_act
                                getItemIds: function () {
                                    return root.artists?.map(el => QA.Util.artistId(el.id_proto));
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
                        MD.BusyIconButton {
                            readonly property QA.FavoriteAction favoriteAction: QA.FavoriteAction {
                                itemId: root.currentSongId
                            }
                            MD.ActionCommand.action: favoriteAction
                            enabled: root.currentSongId.valid && MD.ActionCommand.enabled
                            text: MD.ActionCommand.text
                            icon.actionIcon: MD.ActionCommand.icon
                            icon.fill: favoriteAction.checked
                            mdState.selected: favoriteAction.checked
                            mdState.toggleable: favoriteAction.checkable
                            busy: favoriteAction.busy
                            progress: favoriteAction.progress
                        }
                        MD.IconButton {
                            enabled: QA.App.playqueue.canPrev
                            icon.name: MD.Token.icon.skip_previous

                            onClicked: QA.App.playqueue.prev()
                        }
                        MD.IconButton {
                            mdState.type: MD.Enum.IBtFilled
                            mdState.selected: QA.Global.player.playing
                            icon.fill: QA.Global.player.playing
                            icon.name: QA.Global.player.playing ? MD.Token.icon.pause : MD.Token.icon.play_arrow
                            onClicked: QA.Action.toggle()
                        }
                        MD.IconButton {
                            enabled: QA.App.playqueue.canNext
                            icon.name: MD.Token.icon.skip_next

                            onClicked: QA.App.playqueue.next()
                        }
                        MD.IconButton {
                            icon.name: QA.Util.loopModeIcon(QA.App.playqueue.loopMode)

                            onClicked: {
                                QA.App.playqueue.iterLoopMode();
                            }
                        }
                        MD.IconButton {
                            icon.name: MD.Token.icon.playlist_play

                            onClicked: {
                                QA.Action.openPopup(QA.Enum.SRQueue);
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
                                MD.Util.showPopup(comp_song_menu, {
                                    "song": root.currentSong,
                                    "itemId": root.currentSongId,
                                    "anchorItem": this,
                                    "preferredPlacement": MD.PopupBase.Above
                                }, this);
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
                                text: QA.Util.formatDuration(QA.Global.player.duration * slider.position)
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
                                text: QA.Util.formatDuration(QA.Global.player.duration)
                            }
                        }
                    }
                }
                Component {
                    id: comp_ctl_small
                    RowLayout {
                        MD.IconButton {
                            icon.name: QA.Global.player.playing ? MD.Token.icon.pause : MD.Token.icon.play_arrow

                            onClicked: QA.Action.toggle()

                            MD.CircularIndicator {
                                anchors.centerIn: parent
                                indeterminate: false
                                value: QA.Global.player.duration > 0 ? QA.Global.player.position / QA.Global.player.duration : 0
                            }
                        }
                        MD.IconButton {
                            icon.name: MD.Token.icon.playlist_play

                            onClicked: {
                                QA.Action.openPopup(QA.Enum.SRQueue);
                            }
                        }
                    }
                }
                MD.PageContainer {
                    id: ctl_stack
                    function switch_to() {
                        switchTo(root.isSmall ? comp_ctl_small : comp_ctl, {}, false);
                    }

                    Layout.fillHeight: true

                    Binding on implicitWidth {
                        value: ctl_stack.currentItem.implicitWidth
                        when: ctl_stack.currentItem
                    }
                    initialItem: Item {}
                    replaceMotion: Component {
                        MD.PageMotion {
                            id: motion
                            timeline: MD.SeekableTransition {
                                MD.RealTrack {
                                    id: progress
                                    values: ({
                                            "from": 0,
                                            "to": 1
                                        })
                                    spec: MD.Motion.tween(200, ({
                                            type: Easing.OutCubic
                                        }))
                                }
                            }
                            enterOffset: Qt.point(0, (1 - progress.value) * context.viewport.height * 0.5)
                            enterOpacity: progress.value
                            exitOpacity: 1 - progress.value
                        }
                    }

                    Component.onCompleted: {
                        root.isSmallChanged.connect(switch_to);
                        root.isSmallChanged();
                    }
                }
            }
        }
    }
}

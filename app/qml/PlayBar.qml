pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Pane {
    id: root

    readonly property var currentSong: QA.App.playqueue.currentSong
    readonly property var artists: {
        const ex = QA.Store.extra(root.currentSong.itemId);
        return ex?.artists ?? [];
    }

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
                    source: QA.Util.image_url(root.currentSong.itemId)
                    sourceSize.height: 48
                    sourceSize.width: 48
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
                            cursorShape: Qt.PointingHandCursor
                            hoverEnabled: true

                            onClicked: {
                                QA.Action.routeItem(root.currentSong.albumId);
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
                                    return root.artists?.map(el => QA.Util.artistId(el.id));
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
                            enabled: root.currentSong.itemId.valid
                            action: QA.FavoriteAction {
                                itemId: root.currentSong.itemId
                            }
                        }
                        MD.IconButton {
                            enabled: QA.App.playqueue.canPrev
                            icon.name: MD.Token.icon.skip_previous

                            onClicked: QA.App.playqueue.prev()
                        }
                        MD.IconButton {
                            type: MD.Enum.IBtFilled
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
                                const popup = MD.Util.showPopup(comp_song_menu, {
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

                            MD.CircleProgressBar {
                                anchors.centerIn: parent
                                value: (QA.Global.player.position / QA.Global.player.duration)
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

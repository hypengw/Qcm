import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Shapes
import QcmApp
import "./component"
import "./part"

Pane {
    id: root

    readonly property bool is_small: root.width > 0 && root.width < 650

    Material.elevation: 4
    padding: 0

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PlaySlider {
            id: slider
            Layout.fillWidth: true
            visible: !is_small
        }
        RowLayout {
            Layout.bottomMargin: 8
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.topMargin: 8

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
            Component {
                id: comp_ctl
                RowLayout {
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
                    }
                    Label {
                        readonly property date position: new Date(QA.player.duration * slider.position)

                        text: `${Qt.formatDateTime(position, 'mm:ss')} / ${Qt.formatDateTime(QA.player.duration_date, 'mm:ss')}`
                    }
                }
            }
            Component {
                id: comp_ctl_small
                RowLayout {
                    MRoundButton {
                        flat: true
                        icon.name: QA.player.playing ? Theme.ic.pause : Theme.ic.play_arrow

                        onClicked: {
                            const player = QA.player;
                            if (player.playing)
                                player.pause();
                            else
                                player.play();
                        }

                        Item {
                            anchors.fill: parent
                            layer.enabled: true
                            layer.samples: 8

                            Shape {
                                id: shape_play_arc
                                anchors.centerIn: parent

                                ShapePath {
                                    capStyle: ShapePath.RoundCap
                                    fillColor: "transparent"
                                    startX: 0
                                    startY: 0
                                    strokeColor: Theme.color.primary
                                    strokeWidth: 4

                                    PathAngleArc {
                                        centerX: 0
                                        centerY: 0
                                        radiusX: 16
                                        radiusY: radiusX
                                        startAngle: -90
                                        sweepAngle: 360 * (QA.player.position / QA.player.duration)
                                    }
                                }
                            }
                        }
                    }
                    MRoundButton {
                        flat: true
                        icon.name: Theme.ic.playlist_play

                        onClicked: {
                            pop_playlist.open();
                        }
                    }
                }
            }
            StackView {
                id: ctl_stack
                function switch_to() {
                    replace(currentItem, root.is_small ? comp_ctl_small : comp_ctl);
                }

                Layout.fillHeight: true

                Binding on implicitWidth  {
                    value: ctl_stack.currentItem.implicitWidth
                    when: ctl_stack.currentItem
                }
                initialItem: Item {
                }
                replaceEnter: Transition {
                    LineAnimation {
                        from: 0.5 * ctl_stack.height
                        property: 'y'
                        to: 0
                    }
                    FadeIn {
                    }
                }
                replaceExit: Transition {
                    FadeOut {
                    }
                }

                Component.onCompleted: {
                    root.is_smallChanged.connect(switch_to);
                    root.is_smallChanged();
                }

                PagePopup {
                    id: pop_playlist
                    source: 'qrc:/QcmApp/qml/page/PlayQueuePage.qml'
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

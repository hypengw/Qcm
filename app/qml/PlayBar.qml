pragma ComponentBehavior: Bound
import QtQuick
import Qcm.App as QA
import Qcm.Msg as QM
import Qcm.Material as MD

MD.Pane {
    id: root
    objectName: "playBar"

    property Item backdropSource
    property int backgroundStyle: QA.Global.playbar_background_style
    property bool floating: true
    property real layoutWidth: width
    readonly property bool isSmall: layoutWidth < 650
    readonly property var currentSong: QA.App.playqueue.currentSong
    readonly property QA.item_id currentSongId: QA.App.playqueue.currentSongId
    readonly property QA.EntityView entity: QA.Store.item(currentSongId)
    readonly property QA.item_id currentAlbumId: entity?.albumId ?? QA.Util.createItemid()
    readonly property list<var> artists: entity?.artists ?? []

    implicitHeight: floating ? (isSmall ? 72 : 104) : 64
    padding: 0
    radius: floating ? 24 : 0
    backgroundColor: MD.MProp.color.surface_container
    MD.MProp.textColor: MD.MProp.color.on_surface
    MD.MProp.color: playBarBackground.usingCover ? coverColors : (parent ? parent.MD.MProp.color : MD.Token.color)

    MD.MdColorMgr {
        id: coverColors
        useSysColorSM: false
        accentColor: playBarBackground.accentColor
        mode: MD.Enum.Dark
    }

    background: Item {
        MD.Elevation {
            anchors.fill: parent
            visible: root.floating
            corners: MD.Util.corners(root.radius)
            elevation: MD.Token.elevation.level2
        }
        QA.PlayBarBackground {
            id: playBarBackground
            objectName: "playBarBackground"
            anchors.fill: parent
            style: root.backgroundStyle
            sourceItem: root.backdropSource
            coverSource: root.currentSongId.valid ? QA.Util.image_url(root.currentSongId) : ""
            changingTrack: QA.Global.player.changingTrack
            active: root.floating && root.visible && root.opacity > 0
            radius: root.radius
        }
        Rectangle {
            anchors.fill: parent
            visible: root.floating
            radius: root.radius
            color: "transparent"
            border.width: 1
            border.color: Qt.rgba(root.MD.MProp.color.outline.r, root.MD.MProp.color.outline.g, root.MD.MProp.color.outline.b, 0.12)
        }
    }

    component FavoriteButton: MD.BusyIconButton {
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
    component PlayButton: MD.IconButton {
        mdState.type: MD.Enum.IBtFilled
        mdState.selected: QA.Global.player.playing
        icon.fill: true
        icon.name: QA.Global.player.playing ? MD.Token.icon.pause : MD.Token.icon.play_arrow
        Accessible.name: QA.Global.player.playing ? qsTr("Pause") : qsTr("Play")
        onClicked: QA.Action.toggle()
    }

    contentItem: Item {
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: QA.Action.togglePlaybar()
        }

        Item {
            id: metadata
            x: 16
            y: (parent.height - height) / 2
            width: root.isSmall ? auxiliary.x - x - 12 : Math.max(170, Math.min(300, root.layoutWidth * 0.29))
            height: root.isSmall ? 48 : 56
            QA.Image {
                id: cover
                displaySize: Qt.size(metadata.height, metadata.height)
                radius: 10
                source: QA.Util.image_url(root.currentSongId)
            }
            Column {
                x: cover.width + 12
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(0, parent.width - x - (favorite.visible ? favorite.width + 4 : 0))
                spacing: 2
                MD.Text {
                    width: parent.width
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    typescale: MD.Token.typescale.body_medium
                    text: root.currentSong.name
                    MD.MProp.textColor: albumLink.containsMouse ? root.MD.MProp.color.primary : root.MD.MProp.textColor
                    MouseArea {
                        id: albumLink
                        anchors.fill: parent
                        enabled: root.currentAlbumId.valid
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: QA.Action.routeItem(root.currentAlbumId)
                    }
                }
                MD.ActionLabel {
                    width: parent.width
                    maximumLineCount: 1
                    typescale: MD.Token.typescale.body_small
                    opacity: playBarBackground.usingCover ? 0.9 : 0.7
                    action: MD.Action {
                        text: QA.Util.joinName(root.artists, '/')
                        onTriggered: artistAction.trigger()
                    }
                    QA.GoToArtistAction {
                        id: artistAction
                        artistIds: QM.Ids.artistIds(root.artists.map(el => el.id_proto))
                    }
                }
                Row {
                    spacing: 2
                    visible: children.length > 0
                    Repeater {
                        model: root.currentSong.tags
                        QA.SongTag {
                            required property var modelData
                            tag: modelData
                        }
                    }
                }
            }
            FavoriteButton {
                id: favorite
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                visible: !root.isSmall && root.layoutWidth >= 820
            }
        }

        Item {
            id: center
            visible: !root.isSmall
            x: metadata.x + metadata.width + 12
            width: Math.max(0, auxiliary.x - x - 12)
            height: parent.height
            Row {
                id: playbackControls
                anchors.horizontalCenter: parent.horizontalCenter
                y: 4
                height: 52
                spacing: 4
                MD.IconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: QA.App.playqueue.canPrev
                    icon.name: MD.Token.icon.skip_previous
                    Accessible.name: qsTr("Previous")
                    onClicked: QA.App.playqueue.prev()
                }
                PlayButton {
                    objectName: "playButton"
                    width: 64
                    height: 52
                }
                MD.IconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: QA.App.playqueue.canNext
                    icon.name: MD.Token.icon.skip_next
                    Accessible.name: qsTr("Next")
                    onClicked: QA.App.playqueue.next()
                }
            }
            Item {
                y: 56
                width: parent.width
                height: 48
                MD.Text {
                    id: elapsed
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: QA.Util.formatDuration(QA.Global.player.duration * slider.position)
                    typescale: MD.Token.typescale.label_small
                }
                QA.PlaySlider {
                    id: slider
                    objectName: "playBarSlider"
                    anchors.left: elapsed.right
                    anchors.right: duration.left
                    anchors.margins: 8
                    height: parent.height
                    handle: null
                    wavy: true
                }
                MD.Text {
                    id: duration
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: QA.Util.formatDuration(QA.Global.player.duration)
                    typescale: MD.Token.typescale.label_small
                }
            }
        }

        Row {
            id: auxiliary
            x: parent.width - width - 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0
            PlayButton {
                objectName: "smallPlayButton"
                visible: root.isSmall
                mdState.type: root.MD.MProp.size.isCompact ? MD.Enum.IBtStandard : MD.Enum.IBtFilled
                mdState.selected: !root.MD.MProp.size.isCompact && QA.Global.player.playing
                icon.fill: !root.MD.MProp.size.isCompact
                MD.CircularIndicator {
                    anchors.centerIn: parent
                    indeterminate: false
                    value: QA.Global.player.duration > 0 ? QA.Global.player.position / QA.Global.player.duration : 0
                }
            }
            MD.IconButton {
                visible: !root.isSmall && root.layoutWidth >= 760
                icon.name: QA.Util.loopModeIcon(QA.App.playqueue.loopMode)
                Accessible.name: qsTr("Playback order")
                onClicked: QA.App.playqueue.iterLoopMode()
            }
            MD.IconButton {
                icon.name: MD.Token.icon.playlist_play
                Accessible.name: qsTr("Queue")
                onClicked: QA.Action.openPopup(QA.Enum.SRQueue)
            }
            QA.VolumeButton {
                visible: !root.isSmall
                volume: QA.Global.player.volume
                onVolumeSeted: volume_ => QA.Global.player.volume = volume_
            }
            MD.IconButton {
                visible: !root.isSmall
                icon.name: MD.Token.icon.more_vert
                Accessible.name: qsTr("More")
                onClicked: MD.Util.showPopup(songMenu, {
                    "itemId": root.currentSongId,
                    "anchorItem": this,
                    "preferredPlacement": MD.PopupBase.Above
                }, this)
                Component {
                    id: songMenu
                    QA.SongMenu {}
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    showBackground: true
    backgroundColor: QA.Global.playing_color
    readonly property var song: QA.App.playqueue.currentSong
    readonly property list<var> artists: {
        const ex = QA.Store.extra(song.itemId);
        return ex?.artists ?? [];
    }

    background: Rectangle {
        id: m_background
        color: root.backgroundColor //m_color_mgr.accentColor
        border.width: 0
        MD.Shape {
            MD.RectPath {
                strokeColor: "transparent"
                width: m_background.width
                height: m_background.height
                fillGradient: LinearGradient {
                    y2: m_background.height

                    GradientStop {
                        position: 0.0
                        color: Qt.alpha("#000000", 0)
                    }
                    GradientStop {
                        position: 1.0
                        color: Qt.alpha("#000000", 0.5)
                    }
                }
            }
        }
    }

    MD.MProp.backgroundColor: "transparent"
    MD.MProp.textColor: MD.MProp.color.on_surface
    MD.MProp.color: MD.MdColorMgr {
        id: m_color_mgr
        useSysColorSM: false
        accentColor: QA.Global.playing_color
        mode: MD.Enum.Dark
    }

    header: MD.Pane {
        clip: false
        implicitHeight: 0

        RowLayout {
            MD.IconButton {
                id: m_back
                Layout.alignment: Qt.AlignLeft
                Layout.leftMargin: 12
                Layout.topMargin: 12

                action: MD.Action {
                    icon.name: MD.Token.icon.arrow_back

                    onTriggered: {
                        if (root.canBack) {
                            root.back();
                        } else {
                            QA.Action.togglePlaybar();
                        }
                    }
                }
            }
        }
    }

    property bool canBack: m_small.visible && m_small.depth > 1

    function back() {
        m_small.switch_pane();
    }

    Item {
        visible: false
        MD.Pane {
            id: play_pane
            property int minimumWidth: row_slider.implicitWidth
            Layout.fillWidth: true
            contentHeight: contentChildren[1].implicitHeight
            contentWidth: contentChildren[1].implicitWidth
            background: null

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (m_small.visible)
                        m_small.switch_pane();
                }
            }

            ColumnLayout {
                z: 1
                anchors.centerIn: parent
                spacing: 12
                width: parent.width
                QA.Image {
                    Layout.alignment: Qt.AlignHCenter
                    elevation: MD.Token.elevation.level2
                    source: QA.Util.image_url(root.song.itemId)
                    radius: 16

                    Layout.preferredWidth: displaySize.width
                    Layout.preferredHeight: displaySize.height
                    displaySize: Qt.size(size, size)
                    fixedSize: false
                    readonly property real size: Math.max(240, (root.Window.window?.width ?? 8) / 4.0)
                    property url lastPickSource

                    function pickColor() {
                        const src = source;
                        if (lastPickSource != src) {
                            if (status == Image.Ready && visible) {
                                inner.grabToImage(function (res) {
                                    QA.Notifier.specialImageLoaded('playing', res.image);
                                    lastPickSource = src;
                                });
                            }
                        }
                    }

                    onStatusChanged: pickColor()
                    onVisibleChanged: pickColor()
                }
                MD.Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 12
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    font.bold: true
                    typescale: MD.Token.typescale.title_large
                    horizontalAlignment: Text.AlignHCenter
                    text: root.song.name
                }
                MD.Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                    typescale: MD.Token.typescale.body_medium
                    horizontalAlignment: Text.AlignHCenter
                    text: root.song.albumName
                }
                MD.Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                    typescale: MD.Token.typescale.body_medium
                    horizontalAlignment: Text.AlignHCenter
                    text: QA.Util.joinName(root.artists, '/')
                }
                RowLayout {
                    id: row_control
                    Layout.alignment: Qt.AlignHCenter

                    MD.IconButton {
                        action: QA.FavoriteAction {
                            itemId: root.song.itemId
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

                        onClicked: {
                            const player = QA.Global.player;
                            if (player.playing)
                                player.pause();
                            else
                                player.play();
                        }
                    }
                    MD.IconButton {
                        enabled: QA.App.playqueue.canNext
                        icon.name: MD.Token.icon.skip_next

                        onClicked: QA.App.playqueue.next()
                    }
                    MD.IconButton {
                        flat: true
                        icon.name: QA.Util.loopModeIcon(QA.App.playqueue.loopMode)

                        onClicked: QA.App.playqueue.iterLoopMode()
                    }
                }
                RowLayout {
                    id: row_slider
                    Layout.alignment: Qt.AlignHCenter

                    MD.FontMetrics {
                        id: fm_time
                        typescale: MD.Token.typescale.body_medium
                    }
                    MD.Text {
                        Layout.preferredWidth: fm_time.rect("00:00").width + 2
                        horizontalAlignment: Qt.AlignHCenter
                        readonly property date position: new Date()

                        opacity: QA.Global.player.duration > 0 ? 1 : 0
                        text: QA.Util.formatDuration(QA.Global.player.duration * slider.position, 'mm:ss')
                    }
                    QA.PlaySlider {
                        id: slider
                        Layout.preferredWidth: 220
                    }
                    MD.Text {
                        Layout.preferredWidth: fm_time.rect("00:00").width + 2
                        horizontalAlignment: Qt.AlignHCenter
                        opacity: QA.Global.player.duration > 0 ? 1 : 0
                        text: QA.Util.formatDuration(QA.Global.player.duration, 'mm:ss')
                    }
                }

                RowLayout {
                    Layout.preferredWidth: row_control.width
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: false
                    Item {
                        Layout.fillWidth: true
                    }
                    MD.IconButton {
                        icon.name: MD.Token.icon.playlist_play

                        onClicked: {
                            QA.Action.openPopup(QA.Enum.SRQueue);
                        }
                    }
                    MD.IconButton {
                        action: QA.CommentAction {
                            itemId: root.song.itemId
                        }
                    }
                    MD.IconButton {
                        icon.name: MD.Token.icon.more_vert

                        onClicked: {
                            const popup = MD.Util.showPopup('qrc:/Qcm/App/qml/menu/SongMenu.qml', {
                                "song": root.song,
                                "y": 0
                            }, this);
                            popup.y = -popup.height;
                        }
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
            }
        }
        MD.Pane {
            id: lyric_pane
            padding: 0
            Layout.fillHeight: true
            Layout.fillWidth: true
            implicitWidth: play_pane.implicitWidth
            background: null

            QA.LyricQuery {
                id: m_query
                itemId: root.song.itemId
            }

            Connections {
                target: m_query.data
                function onCurrentIndexChanged(idx) {
                    lyric_view.posTo(idx);
                }
            }
            Binding {
                target: m_query.data
                property: 'position'
                when: !QA.Global.player.busy
                value: QA.Global.player.position + 50
                restoreMode: Binding.RestoreNone
            }

            QA.LyricView {
                id: lyric_view
                anchors.fill: parent
                model: m_query.data
            }
        }
    }

    onWidthChanged: {
        if (width < (play_pane.minimumWidth + 32) * 2) {
            m_large.visible = false;
            m_small.visible = true;
        } else {
            m_small.visible = false;
            m_large.visible = true;
        }
    }

    RowLayout {
        id: m_large
        visible: false
        anchors.fill: parent
        LayoutItemProxy {
            target: play_pane
        }
        LayoutItemProxy {
            target: lyric_pane
        }
    }

    MD.StackView {
        id: m_small
        visible: true
        anchors.fill: parent

        function push_pane(target) {
            push(m_layout_comp, {
                'target': target
            });
        }

        function switch_pane() {
            if (depth > 1) {
                popToIndex(0);
            } else {
                push_pane(lyric_pane);
            }
        }

        Component.onCompleted: {
            push_pane(play_pane);
        }

        Component {
            id: m_layout_comp
            ColumnLayout {
                property alias target: m_stack_layout_proxy.target
                LayoutItemProxy {
                    id: m_stack_layout_proxy
                }
            }
        }
    }
}

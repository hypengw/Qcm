import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD
import "../js/util.mjs" as Util

MD.Page {
    id: root

    showBackground: true
    backgroundColor: MD.MatProp.color.surface

    header: MD.Pane {
        clip: false
        implicitHeight: 0

        RowLayout {
            MD.IconButton {
                id: m_back
                Layout.alignment: Qt.AlignLeft
                Layout.leftMargin: 12
                Layout.topMargin: 12

                action: Action {
                    icon.name: MD.Token.icon.arrow_back

                    onTriggered: {
                        if (root.canBack) {
                            root.back();
                        } else {
                            QA.Action.route_special('main');
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

    QNcm.SongLyricQuerier {
        id: querier_lyric

        readonly property string combined_lrc: {
            return data.lrc + data.transLrc;
        }

        autoReload: songId.valid()
        songId: QA.Global.cur_song.itemId
    }

    Item {
        visible: false
        MD.Pane {
            id: play_pane
            property int minimumWidth: row_slider.implicitWidth
            Layout.fillWidth: true
            contentHeight: contentChildren[1].implicitHeight
            contentWidth: contentChildren[1].implicitWidth

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
                    source: QA.Util.image_url(QA.Global.cur_song.album.picUrl)
                    radius: 16

                    Layout.preferredWidth: displaySize.width
                    Layout.preferredHeight: displaySize.height
                    displaySize: Qt.size(size, size)
                    fixedSize: false
                    readonly property real size: Math.max(240, (root.Window.window?.width ?? 8) / 4.0)
                }
                MD.Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 12
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    font.bold: true
                    typescale: MD.Token.typescale.title_large
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
                    id: row_control
                    Layout.alignment: Qt.AlignHCenter

                    MD.IconButton {
                        checked: QA.Global.session.user.collection.contains(QA.Global.cur_song.itemId)
                        enabled: QA.Global.cur_song.itemId.valid()
                        icon.name: checked ? MD.Token.icon.favorite : MD.Token.icon.favorite_border

                        onClicked: {
                            QA.Action.collect(QA.Global.cur_song.itemId, !checked);
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

                        onClicked: {
                            const player = QA.Global.player;
                            if (player.playing)
                                player.pause();
                            else
                                player.play();
                        }
                    }
                    MD.IconButton {
                        enabled: QA.App.playlist.canNext
                        icon.name: MD.Token.icon.skip_next

                        onClicked: QA.App.playlist.next()
                    }
                    MD.IconButton {
                        flat: true
                        icon.name: QA.Global.loop_icon

                        onClicked: QA.App.playlist.iterLoopMode()
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
                        readonly property date position: new Date(QA.Global.player.duration * slider.position)

                        opacity: QA.Global.player.duration > 0 ? 1 : 0
                        text: `${Qt.formatDateTime(position, 'mm:ss')}`
                    }
                    QA.PlaySlider {
                        id: slider
                        Layout.preferredWidth: 220
                    }
                    MD.Text {
                        Layout.preferredWidth: fm_time.rect("00:00").width + 2
                        horizontalAlignment: Qt.AlignHCenter
                        opacity: QA.Global.player.duration > 0 ? 1 : 0
                        text: `${Qt.formatDateTime(QA.Global.player.durationDate, 'mm:ss')}`
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
                            QA.Action.popup_special(QA.enums.SRQueue);
                        }
                    }
                    MD.IconButton {
                        action: QNcm.CommentAction {
                            itemId: QA.Global.cur_song.itemId
                        }
                    }
                    MD.IconButton {
                        icon.name: MD.Token.icon.more_vert

                        onClicked: {
                            const popup = MD.Util.show_popup('qrc:/Qcm/App/qml/menu/SongMenu.qml', {
                                "song": QA.Global.cur_song,
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

            QA.LrcLyric {
                id: lrc
                Binding on position {
                    when: !QA.Global.player.busy
                    // offset to avoid flicking
                    value: QA.Global.player.position + 50
                    restoreMode: Binding.RestoreNone
                }
                source: querier_lyric.combined_lrc

                onCurrentIndexChanged: {
                    lyric_view.posTo(currentIndex);
                }
            }
            QA.LyricView {
                id: lyric_view
                anchors.fill: parent
                model: lrc
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

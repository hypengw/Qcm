import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.ListItem {
    id: root

    readonly property bool isPlaying: QA.Global.playlist.cur.itemId === dgModel.itemId
    property QA.t_song dgModel: modelData
    property string subtitle: ''
    property bool showCover: false
    readonly property int coverSize: 48

    enabled: dgModel.canPlay
    highlighted: isPlaying
    heightMode: MD.Enum.ListItemTwoLine

    rightPadding: 0

    radius: indexRadius(dgIndex, count, 16)

    mdState: MD.StateListItem {
        item: root
        backgroundColor: ctx.color.surface_container
    }

    divider: MD.Divider {
        anchors.bottom: parent.bottom
        leftMargin: 16
        rightMargin: 16
    }

    contentItem: RowLayout {
        spacing: 16

        Item {
            Layout.minimumWidth: {
                const text_with = item_font_metrics.advanceWidth(root.count.toString()) + 2;
                if (root.showCover) {
                    return Math.max(text_with, root.coverSize);
                }
                return text_with;
            }
            implicitWidth: children[0].implicitWidth
            implicitHeight: children[0].implicitHeight
            StackLayout {
                anchors.fill: parent
                Layout.fillHeight: false
                Layout.fillWidth: false

                currentIndex: root.showCover ? 2 : 0

                Binding on currentIndex {
                    value: 1
                    when: root.isPlaying
                }

                MD.FontMetrics {
                    id: item_font_metrics
                    typescale: MD.Token.typescale.body_medium
                }

                MD.Text {
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                    typescale: MD.Token.typescale.body_medium
                    opacity: 0.6
                    text: dgIndex + 1
                }
                MD.Icon {
                    name: MD.Token.icon.equalizer
                    size: 24
                    MD.MatProp.textColor: MD.Token.color.primary
                    horizontalAlignment: Qt.AlignHCenter
                }
                Loader {
                    active: root.showCover
                    sourceComponent: m_comp_song_image
                }
                Component {
                    id: m_comp_song_image
                    QA.Image {
                        radius: 8
                        source: `image://ncm/${root.dgModel.album.picUrl}`
                        displaySize: Qt.size(48, 48)
                    }
                }
            }
        }
        ColumnLayout {
            spacing: 0
            MD.Text {
                Layout.fillWidth: true
                text: root.dgModel.name
                color: root.isPlaying ? MD.Token.color.primary : MD.MatProp.textColor
                typescale: MD.Token.typescale.body_large
                verticalAlignment: Qt.AlignVCenter
            }
            RowLayout {
                Repeater {
                    model: root.dgModel.tags

                    delegate: ColumnLayout {
                        QA.SongTag {
                            tag: modelData
                        }
                    }
                }
                MD.Text {
                    id: subtitle_label
                    Layout.fillWidth: true
                    verticalAlignment: Qt.AlignVCenter
                    typescale: MD.Token.typescale.body_medium
                    color: root.mdState.supportTextColor
                    text: root.subtitle ? root.subtitle : `${QA.Global.join_name(root.dgModel.artists, '/')} - ${root.dgModel.album.name}`
                }
            }
        }
        MD.Text {
            visible: !(Window.window?.windowClass === MD.Enum.WindowClassCompact)
            typescale: MD.Token.typescale.body_medium
            text: Qt.formatDateTime(root.dgModel.duration, 'mm:ss')
            verticalAlignment: Qt.AlignVCenter
        }
        RowLayout {
            spacing: 0

            MD.IconButton {
                visible: !(Window.window?.windowClass === MD.Enum.WindowClassCompact)
                checked: QA.Global.user_song_set.contains(root.dgModel.itemId)
                icon.name: checked ? MD.Token.icon.favorite : MD.Token.icon.favorite_border

                onClicked: {
                    QA.Global.querier_user_song.like_song(root.dgModel.itemId, !checked);
                }
            }
            MD.IconButton {
                icon.name: MD.Token.icon.more_vert

                onClicked: {
                    MD.Util.show_popup('qrc:/Qcm/App/qml/menu/SongMenu.qml', {
                        "song": dgModel,
                        "y": height
                    }, this);
                }
            }
        }
    }
}

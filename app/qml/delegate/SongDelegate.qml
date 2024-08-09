import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.ListItem {
    id: root

    readonly property bool isPlaying: QA.Global.playlist.cur.itemId === model_.itemId
    property QA.t_song model_: modelData
    property string subtitle: ''
    property bool showCover: false
    readonly property int coverSize: 48

    enabled: model_.canPlay
    highlighted: isPlaying
    heightMode: MD.Enum.ListItemTwoLine

    rightPadding: 16

    radius: [index_ == 0 ? 16 : 0, index_ + 1 == count ? 16 : 0]

    mdState: MD.StateListItem {
        item: root
        backgroundColor: ctx.color.surface_container
    }

    divider: MD.Divider {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        full: false
        height: 1
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
                    text: index_ + 1
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
                        source: `image://ncm/${root.model_.album.picUrl}`
                        displaySize: Qt.size(48, 48)
                    }
                }
            }
        }
        ColumnLayout {
            spacing: 0
            MD.Text {
                Layout.fillWidth: true
                text: root.model_.name
                color: root.isPlaying ? MD.Token.color.primary : MD.MatProp.textColor
                typescale: MD.Token.typescale.body_large
                verticalAlignment: Qt.AlignVCenter
            }
            RowLayout {
                Repeater {
                    model: root.model_.tags

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
                    text: root.subtitle ? root.subtitle : `${QA.Global.join_name(root.model_.artists, '/')} - ${root.model_.album.name}`
                }
            }
        }
        MD.Text {
            visible: !(QA.Global.main_win?.smallLayout ?? false)
            typescale: MD.Token.typescale.body_medium
            text: Qt.formatDateTime(root.model_.duration, 'mm:ss')
            verticalAlignment: Qt.AlignVCenter
        }
        RowLayout {
            spacing: 0

            MD.IconButton {
                visible: !(QA.Global.main_win?.smallLayout ?? false)
                checked: QA.Global.user_song_set.contains(root.model_.itemId)
                icon.name: checked ? MD.Token.icon.favorite : MD.Token.icon.favorite_border

                onClicked: {
                    QA.Global.querier_user_song.like_song(root.model_.itemId, !checked);
                }
            }
            MD.IconButton {
                icon.name: MD.Token.icon.more_vert

                onClicked: {
                    MD.Tool.show_popup('qrc:/Qcm/App/qml/menu/SongMenu.qml', {
                        "song": model_,
                        "y": height
                    }, this);
                }
            }
        }
    }
}

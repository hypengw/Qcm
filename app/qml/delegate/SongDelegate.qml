import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Material as MD

MD.ListItem {
    id: root

    readonly property bool isPlaying: QA.Global.cur_song.itemId === dgModel.itemId
    property var dgModel: {
        // bind visible
        if (visible) {
            if (typeof modelData?.objectName == 'string') {
                return modelData;
            }
            if (model) {
                return model;
            }
        }
        return QA.App.empty.song;
    }
    property string subtitle: ''
    property bool showCover: false
    readonly property int coverSize: 48

    enabled: dgModel.canPlay
    highlighted: isPlaying
    heightMode: MD.Enum.ListItemTwoLine

    rightPadding: rightMargin

    corners: indexCorners(dgIndex, count, 16)

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
            QA.ListenIcon {
                anchors.fill: parent
                Layout.fillHeight: false
                Layout.fillWidth: false

                currentIndex: root.showCover ? 2 : 0
                isPlaying: root.isPlaying
                index: root.dgIndex
                MD.FontMetrics {
                    id: item_font_metrics
                    typescale: MD.Token.typescale.body_medium
                }
                Loader {
                    active: root.showCover
                    sourceComponent: m_comp_song_image
                }
                Component {
                    id: m_comp_song_image
                    QA.Image {
                        radius: 8
                        source: QA.Util.image_url(root.dgModel?.album.picUrl ?? root.dgModel.coverUrl)
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
                checked: QA.Global.session.user.collection.contains(root.dgModel.itemId)
                icon.name: checked ? MD.Token.icon.favorite : MD.Token.icon.favorite_border

                onClicked: {
                    QA.Action.collect(root.dgModel.itemId, !checked);
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

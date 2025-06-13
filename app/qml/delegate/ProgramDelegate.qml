import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.ListItem {
    id: root

    readonly property bool isPlaying: QA.App.playqueue.currentSong.itemId === dgModel.songId
    property var dgModel: modelData
    property string subtitle: ''

    // enabled: dgModel.canPlay
    highlighted: isPlaying
    heightMode: MD.Enum.ListItemTwoLine

    rightPadding: 16

    corners: MD.Util.listCorners(index, count, 16)

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

        QA.ListenIcon {
            Layout.fillHeight: false
            Layout.fillWidth: false
            Layout.minimumWidth: item_font_metrics.advanceWidth(root.count.toString()) + 2

            isPlaying: root.isPlaying
            index: root.dgModel.serialNumber

            MD.FontMetrics {
                id: item_font_metrics
                typescale: MD.Token.typescale.body_medium
            }
        }
        ColumnLayout {
            spacing: 0
            MD.Text {
                Layout.fillWidth: true
                text: root.dgModel.name
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
                    // text: root.subtitle ? root.subtitle : `${Qt.formatDateTime(root.dgModel.createTime, 'yyyy.MM.dd')} - ${QA.Util.joinName(root.dgModel.song.artists, '/')}`
                }
            }
        }
        MD.Text {
            typescale: MD.Token.typescale.body_medium
            text: Qt.formatDateTime(root.dgModel.duration, 'mm:ss')
            verticalAlignment: Qt.AlignVCenter
        }
        RowLayout {
            spacing: 0

            MD.IconButton {
                icon.name: MD.Token.icon.more_vert

                onClicked: {
                    MD.Util.showPopup('qrc:/Qcm/App/qml/menu/ProgramMenu.qml', {
                        "itemId": root.dgModel.itemId,
                        "song": root.dgModel.song,
                        "y": height
                    }, this);
                }
            }
        }
    }
}

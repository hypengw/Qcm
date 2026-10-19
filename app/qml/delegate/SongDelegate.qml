pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Material as MD

MD.ListItem {
    id: root
    required index
    required model

    readonly property bool hasModel: visible && index >= 0 && !!model
    readonly property QA.item_id itemId: hasModel ? model.itemId : QA.Util.createItemid()
    readonly property bool isPlaying: itemId.valid && QA.App.playqueue.currentSongId === itemId
    property var dgModel: hasModel ? model : QA.App.empty.song
    property string subtitle: ''
    property bool showCover: false
    property bool canDelete: false
    property bool useTracknumber: true
    readonly property int coverSize: 48

    enabled: itemId.valid && dgModel.canPlay
    highlighted: isPlaying
    heightMode: MD.Enum.ListItemTwoLine

    rightPadding: rightMargin

    corners: {
        const c = MD.Util.listCorners(index, count, 16);
        if (!prevSameSection) {
            c.topLeft = 16;
            c.topRight = 16;
        }
        if (!nextSameSection) {
            c.bottomLeft = 16;
            c.bottomRight = 16;
        }
        return c;
    }

    mdState.backgroundColor: mdState.ctx.color.surface_container

    divider: MD.AutoDivider {
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
                trackNumber: root.dgModel.trackNumber
                index: root.useTracknumber ? -1 : root.index
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
                        source: QA.Util.image_url(root.itemId)
                        displaySize: Qt.size(48, 48)
                    }
                }
            }
        }
        Column {
            spacing: 0
            Layout.fillWidth: true
            MD.Text {
                width: parent.width
                text: root.dgModel.name
                color: root.isPlaying ? MD.Token.color.primary : MD.MProp.textColor
                typescale: MD.Token.typescale.body_large
                verticalAlignment: Qt.AlignVCenter
            }
            Row {
                width: parent.width
                Repeater {
                    model: root.dgModel.tags

                    delegate: ColumnLayout {
                        required property string modelData
                        QA.SongTag {
                            tag: parent.modelData
                        }
                    }
                }
                MD.Text {
                    id: subtitle_label
                    width: parent.width
                    verticalAlignment: Qt.AlignVCenter
                    typescale: MD.Token.typescale.body_medium
                    color: root.mdState.supportTextColor
                    text: {
                        if (root.subtitle) {
                            return root.subtitle;
                        }
                        if (!root.itemId.valid)
                            return "";
                        const ex = QA.Store.extra(root.itemId);
                        return QA.Util.join([QA.Util.joinName(ex?.artists), ex?.album?.name], ' - ');
                    }
                }
            }
        }
        MD.Text {
            visible: !root.MD.MProp.size.isCompact
            typescale: MD.Token.typescale.body_medium
            text: QA.Util.formatDuration(root.dgModel.duration, 'mm:ss')
            verticalAlignment: Qt.AlignVCenter
        }
        Row {
            spacing: 0

            MD.BusyIconButton {
                visible: !root.MD.MProp.size.isCompact
                readonly property QA.FavoriteAction favoriteAction: QA.FavoriteAction {
                    itemId: root.itemId
                }
                MD.ActionCommand.action: favoriteAction
                enabled: MD.ActionCommand.enabled
                text: MD.ActionCommand.text
                icon.actionIcon: MD.ActionCommand.icon
                icon.fill: favoriteAction.checked
                mdState.selected: favoriteAction.checked
                mdState.toggleable: favoriteAction.checkable
                busy: favoriteAction.busy
                progress: favoriteAction.progress
            }
            MD.IconButton {
                icon.name: MD.Token.icon.more_vert

                onClicked: {
                    const props = {
                        "itemId": root.itemId,
                        "anchorItem": this
                    };
                    if (root.canDelete) {
                        props["canDelete"] = true;
                    }
                    if (root.dgModel.sourceId) {
                        props["sourceId"] = root.dgModel.sourceId;
                    }
                    MD.Util.showPopup('Qcm.App/SongMenu', props, this);
                }
            }
        }
    }
}

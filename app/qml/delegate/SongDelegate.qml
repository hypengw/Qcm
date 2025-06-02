import QtQuick
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
            if (model && model.index >= 0) {
                return model;
            }
        }
        return QA.App.empty.song;
    }
    property string subtitle: ''
    property bool showCover: false
    property bool canDelete: false
    property bool useTracknumber: true
    readonly property int coverSize: 48

    enabled: dgModel.canPlay
    highlighted: isPlaying
    heightMode: MD.Enum.ListItemTwoLine

    rightPadding: rightMargin

    corners: indexCorners(dgIndex, count, 16)

    mdState.backgroundColor: mdState.ctx.color.surface_container

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
                trackNumber: root.dgModel.trackNumber
                index: root.useTracknumber ? -1 : root.dgIndex
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
                        source: QA.Util.image_url(root.dgModel.itemId)
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
                        if (!root.dgModel?.itemId)
                            return "";
                        const ex = QA.Store.extra(root.dgModel.itemId);
                        return [QA.Util.joinName(ex?.artists), ex?.album?.name].filter(e => !!e).join(' - ');
                    }
                }
            }
        }
        MD.Text {
            visible: !root.MD.MProp.size.isCompact
            typescale: MD.Token.typescale.body_medium
            text: QA.Util.formatDateTime(root.dgModel.duration, 'mm:ss')
            verticalAlignment: Qt.AlignVCenter
        }
        Row {
            spacing: 0

            MD.IconButton {
                visible: !root.MD.MProp.size.isCompact
                action: QA.FavoriteAction {
                    itemId: root.dgModel.itemId
                }
            }
            MD.IconButton {
                icon.name: MD.Token.icon.more_vert

                onClicked: {
                    const props = {
                        "itemId": root.dgModel.itemId,
                        "y": height
                    };
                    if (root.canDelete) {
                        props["canDelete"] = true;
                    }
                    if (root.dgModel.sourceId) {
                        props["sourceId"] = root.dgModel.sourceId;
                    }
                    MD.Util.showPopup('qrc:/Qcm/App/qml/menu/SongMenu.qml', props, this);
                }
            }
        }
    }
}

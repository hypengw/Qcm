pragma ComponentBehavior: Bound
import QtQuick
import QtQml.Models as QM
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

MD.Page {
    id: root

    readonly property QA.EntityView entity: QA.Store.item(root.itemId)
    readonly property var album: qr_al.data.album
    property alias albumId: qr_al.albumId
    readonly property QA.item_id itemId: albumId
    title: qsTr("album")

    padding: 0
    scrolling: !view?.atYBeginning

    readonly property var model: qr_al.data
    property Item view: null
    property Item viewHeaderItem: null

    Item {
        visible: false

        QA.Image {
            id: m_cover
            Layout.preferredWidth: displaySize.width
            Layout.preferredHeight: displaySize.height

            displaySize: Qt.size(240, 240)
            elevation: MD.Token.elevation.level2
            source: QA.Util.image_url(root.itemId)
            radius: 16
        }
        MD.Label {
            id: m_title
            maximumLineCount: 2
            text: root.album.name
            typescale: (root.viewHeaderItem?.single ?? false) ? MD.Token.typescale.headline_medium : MD.Token.typescale.headline_large
        }
        RowLayout {
            id: m_info
            spacing: 12
            MD.Label {
                typescale: MD.Token.typescale.body_medium
                text: `${root.album.trackCount} tracks`
            }
            MD.Label {
                typescale: MD.Token.typescale.body_medium
                text: QA.Util.formatDateTime(root.album.publishTime, 'yyyy.MM')
            }
        }
        MD.ActionLabel {
            id: m_artist
            typescale: MD.Token.typescale.body_medium
            action: MD.Action {
                text: {
                    const ex = root.entity;
                    const names = QA.Util.joinName(ex?.artists, '/');
                    return names || qsTr("Unknown Artist");
                }
                onTriggered: {
                    m_go_to_artist_act.trigger();
                }
            }
            QA.GoToArtistAction {
                id: m_go_to_artist_act
                albumArtist: true
                artistIds: {
                    const ex = root.entity;
                    return QM.Ids.artistIds(ex?.artists.map(el => el.id_proto) ?? []);
                }
            }
        }

        QA.ListDescription {
            id: m_desc
            description: root.album.description.trim()
        }
        RowLayout {
            id: m_control_pane
            QA.SortOrderChip {
                Layout.alignment: Qt.AlignVCenter
                model: m_song_sort_type
            }

            Item {
                Layout.fillWidth: true
            }

            MD.ActionIconButton {
                hoverEnabled: true
                MD.ToolTip.text: action?.tooltip || text
                MD.ToolTip.visible: mdState.interactions.hovered && !mdState.interactions.pressed && MD.ToolTip.text.length > 0
                action: QA.AppendListAction {
                    getSongIds: function () {
                        return QA.Util.collect_ids(qr_al.data);
                    }
                }
            }
            MD.BusyIconButton {
                id: btn_fav
                readonly property QA.FavoriteAction favoriteAction: QA.FavoriteAction {
                    itemId: root.itemId
                }
                MD.ActionCommand.action: favoriteAction
                text: MD.ActionCommand.text
                icon.actionIcon: MD.ActionCommand.icon
                enabled: MD.ActionCommand.enabled
                hoverEnabled: true
                MD.ToolTip.text: favoriteAction.tooltip || text
                MD.ToolTip.visible: mdState.interactions.hovered && !mdState.interactions.pressed && MD.ToolTip.text.length > 0
                icon.fill: favoriteAction.checked
                mdState.selected: favoriteAction.checked
                mdState.toggleable: favoriteAction.checkable
                busy: favoriteAction.busy
                progress: favoriteAction.progress
            }
            MD.ActionIconButton {
                id: btn_comment
                // TODO
                visible: false
                action: QA.CommentAction {
                    itemId: root.itemId
                }
            }
        }
    }

    MD.FlickablePane {
        id: m_view_pane
        view: root.view
        excludeBegin: -m_control_pane.height
        radius: root.radius
        includeHorizontalMargins: false
        bottomMargin: root.MD.MProp.page.bottomMargin
        topMargin: 0
    }

    MD.Loader {
        id: m_loader
        anchors.fill: parent
        sourceComponent: m_list_comp
    }

    Component {
        id: m_list_header_comp
        Item {
            id: m_header
            readonly property bool single: width < m_cover.displaySize.width * (1.0 + 1.5) + 8
            width: parent.width
            implicitHeight: children[0].implicitHeight

            Component.onCompleted: root.viewHeaderItem = this

            ColumnLayout {
                anchors.fill: parent
                spacing: 16

                MD.Pane {
                    Layout.fillWidth: true
                    radius: root.radius
                    padding: 16

                    ColumnLayout {
                        width: parent.width
                        spacing: 0
                        RowLayout {
                            spacing: 16
                            visible: !m_header.single

                            LayoutItemProxy {
                                target: m_cover
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                                spacing: 12
                                LayoutItemProxy {
                                    Layout.fillWidth: true
                                    target: m_title
                                }
                                LayoutItemProxy {
                                    target: m_info
                                }
                                LayoutItemProxy {
                                    target: m_artist
                                }
                                LayoutItemProxy {
                                    Layout.fillWidth: true
                                    visible: !!m_desc.description
                                    target: m_desc
                                }
                            }
                        }
                        ColumnLayout {
                            spacing: 0
                            Layout.fillWidth: true
                            visible: m_header.single

                            LayoutItemProxy {
                                Layout.alignment: Qt.AlignHCenter
                                target: m_cover
                            }
                            MD.Space {
                                spacing: 16
                            }
                            ColumnLayout {
                                Layout.alignment: Qt.AlignHCenter
                                spacing: 12

                                LayoutItemProxy {
                                    Layout.alignment: Qt.AlignHCenter
                                    Layout.maximumWidth: implicitWidth
                                    Layout.fillWidth: true
                                    target: m_title
                                }
                                LayoutItemProxy {
                                    Layout.alignment: Qt.AlignHCenter
                                    target: m_info
                                }
                                LayoutItemProxy {
                                    Layout.alignment: Qt.AlignHCenter
                                    Layout.maximumWidth: implicitWidth
                                    Layout.fillWidth: true
                                    target: m_artist
                                }
                                LayoutItemProxy {
                                    Layout.alignment: Qt.AlignHCenter
                                    Layout.fillWidth: true
                                    visible: !!m_desc.description
                                    target: m_desc
                                }
                            }
                        }
                    }
                }

                LayoutItemProxy {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    target: m_control_pane
                }
            }
        }
    }

    Component {
        id: m_list_comp

        MD.PageVerticalLazyList {
            id: m_view
            anchors.fill: parent

            extraBottomMargin: 16

            source: QA.ItemModelSource {
                model: m_sort_filter_model
            }

            header: m_list_header_comp

            delegate: Column {
                id: songRow
                required property MD.LazyRow row
                readonly property var entry: row.value
                readonly property bool firstInDisc: {
                    void row.revision;
                    return row.index === 0 || m_view.source.valueAt(row.index - 1)?.discNumber !== entry.discNumber;
                }
                readonly property bool lastInDisc: {
                    void row.revision;
                    return row.index + 1 === m_view.count || m_view.source.valueAt(row.index + 1)?.discNumber !== entry.discNumber;
                }
                MD.Control {
                    width: parent.width
                    visible: root.model.discCount > 1 && songRow.firstInDisc
                    verticalPadding: 4
                    leftPadding: 24
                    contentItem: MD.Text {
                        text: qsTr('Disc: ') + songRow.entry.discNumber
                        typescale: MD.Token.typescale.title_medium
                    }
                }
                QA.SongDelegate {
                    row: songRow.row
                    width: parent.width
                    leftMargin: 16
                    rightMargin: 16
                    prevSameSection: !songRow.firstInDisc
                    nextSameSection: !songRow.lastInDisc
                    subtitle: QA.Util.joinName(model.artists, '/')
                    onClicked: QA.Action.play(itemId)
                }
            }

            footer: MD.ListBusyFooter {
                running: qr_al.querying

            }

            Component.onCompleted: {
                root.view = m_view;
            }

        }
    }


    MD.FAB {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: root.MD.MProp.page.rightMargin + 16
        anchors.bottomMargin: root.MD.MProp.page.bottomMargin + 16
        flickable: root.view
        icon.name: MD.Token.icon.play_arrow
        onClicked: QA.Action.switch_songs(QA.Util.collect_ids(qr_al.data))
    }

    QA.SongSortFilterModel {
        id: m_sort_filter_model
        sourceModel: root.model
        sortType: m_song_sort_type.currentType
        asc: m_song_sort_type.asc
    }

    QA.SongSortTypeModel {
        id: m_song_sort_type
    }

    QA.AlbumQuery {
        id: qr_al
        Component.onCompleted: reload()
    }
}

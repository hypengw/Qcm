pragma ComponentBehavior: Bound
import QtQuick
import QtQml.Models as QM
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root

    readonly property var album: qr_al.data.album
    property QA.item_id itemId
    title: qsTr("album")

    padding: 0
    scrolling: !view?.atYBeginning

    readonly property var model: qr_al.data
    property Flickable view: null
    property Item viewHeaderItem: null

    Item {
        visible: false

        QA.Image {
            id: m_cover
            Layout.preferredWidth: displaySize.width
            Layout.preferredHeight: displaySize.height

            displaySize: Qt.size(240, 240)
            elevation: MD.Token.elevation.level2
            source: QA.Util.image_url(root.album.itemId)
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
                    const names = QA.Util.joinName(root.model.extra?.artists, '/');
                    return names || qsTr("Unknown Artist");
                }
                onTriggered: {
                    m_go_to_artist_act.trigger();
                }
            }
            QA.GoToArtistAction {
                id: m_go_to_artist_act
                getItemIds: function () {
                    return root.model.extra?.artists.map(el => QA.Util.albumArtistId(el.id));
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

            MD.IconButton {
                action: QA.AppendListAction {
                    getSongIds: function () {
                        return QA.Util.collect_ids(qr_al.data);
                    }
                }
            }
            MD.BusyIconButton {
                id: btn_fav
                action: QA.FavoriteAction {
                    itemId: root.itemId
                }
            }
            MD.IconButton {
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
        excludeBegin: {
            return ((view)?.headerItem?.height ?? 0) - m_control_pane.height + view.topMargin;
        }
        radius: root.radius
        bottomMargin: MD.MProp.size.verticalPadding
    }

    MD.Loader {
        id: m_loader
        anchors.fill: parent
        sourceComponent: m_list_comp // m_table_comp
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

        MD.VerticalListView {
            id: m_view
            anchors.fill: parent
            reuseItems: true
            contentY: 0

            topMargin: MD.MProp.size.verticalPadding
            bottomMargin: MD.MProp.size.verticalPadding * 2

            model: m_sort_filter_model

            header: m_list_header_comp

            section.property: 'discNumber'
            section.criteria: ViewSection.FullString
            section.delegate: root.model.discCount > 1 ? m_list_section_comp : null
            delegate: QA.SongDelegate {
                width: ListView.view.contentWidth
                leftMargin: 16
                rightMargin: 16

                subtitle: QA.Util.joinName(QA.Store.extra(model.itemId)?.artists, '/')

                onClicked: {
                    QA.Action.play(dgModel.itemId);
                }
            }

            footer: MD.ListBusyFooter {
                running: qr_al.querying
                width: ListView.view.contentWidth
            }

            Component.onCompleted: {
                root.view = m_view;
            }
            Component {
                id: m_list_section_comp
                MD.Control {
                    id: m_list_section_root
                    required property string section
                    width: ListView.view.contentWidth
                    height: implicitHeight
                    verticalPadding: 4
                    leftPadding: 16 + 8

                    contentItem: MD.Text {
                        text: qsTr('Disc: ') + m_list_section_root.section
                        typescale: MD.Token.typescale.title_medium
                    }
                }
            }
        }
    }

    Component {
        id: m_table_comp
        Item {
            QA.TableProxyModel {
                id: m_table_model
                sourceModel: m_sort_filter_model
                columnNames: ["name", "albumName", "duration"]
            }
            MD.TableView {
                id: m_view

                readonly property bool single: false
                anchors.fill: parent
                model: m_table_model
                columnWidthProvider: function (column) {
                    let w = explicitColumnWidth(column);
                    if (w >= 0)
                        return w;
                    return width / columns;
                    // return implicitColumnWidth(column);
                }
                topMargin: m_header.height
                delegate: MD.TableViewDelegate {}

                Component.onCompleted: {
                    root.view = m_view;
                }
            }
            MD.HorizontalHeaderView {
                id: m_header
                y: -m_view.contentY - height
                syncView: m_view
            }
        }
    }
    MD.FAB {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 16
        anchors.bottomMargin: 16
        flickable: root.view
        action: MD.Action {
            icon.name: MD.Token.icon.play_arrow
            onTriggered: {
                QA.Action.switch_songs(QA.Util.collect_ids(qr_al.data));
            }
        }
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
        itemId: root.itemId
        Component.onCompleted: reload()
    }
}

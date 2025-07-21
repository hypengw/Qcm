pragma ComponentBehavior: Bound
import QtCore
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Msg as QM
import Qcm.Material as MD

MD.Page {
    id: root

    property var artist: qr_artist.data.item
    property alias itemId: qr_artist.itemId
    readonly property bool single: m_view.width < m_cover.displaySize.width * (1.0 + 1.5) + 8
    property Item viewHeaderItem: null
    property int displayMode: QA.Enum.DCardGrid
    readonly property bool isAlbumArtistId: itemId.type == QA.Enum.ItemAlbumArtist

    title: qsTr("artist")
    padding: 0
    scrolling: !m_view.atYBeginning

    MD.FlickablePane {
        id: m_view_pane
        view: m_view
        excludeBegin: {
            return (root.viewHeaderItem?.height ?? 0) - m_control_pane.height + view.topMargin;
        }
        radius: root.radius
        leftMargin: 0
        rightMargin: 0
        x: m_view.leftMargin
        topMargin: MD.MProp.size.verticalPadding
        bottomMargin: 0
    }
    Item {
        visible: false

        QA.Image {
            id: m_cover
            z: 1
            elevation: MD.Token.elevation.level2
            source: QA.Util.image_url(root.artist.itemId)
            radius: width / 2

            Layout.preferredWidth: displaySize.width
            Layout.preferredHeight: displaySize.height
            displaySize: Qt.size(240, 240)
        }
        MD.Text {
            id: m_title
            maximumLineCount: 2
            text: root.artist.name
            typescale: MD.Token.typescale.headline_medium
        }
        RowLayout {
            id: m_info
            spacing: 12
            // MD.Text {
            //     typescale: MD.Token.typescale.body_medium
            //     text: `${root.artist.albumCount} albums`
            // }
            // MD.Text {
            //     typescale: MD.Token.typescale.body_medium
            //     text: `${root.artist.musicCount} songs`
            // }
        }
        QA.ListDescription {
            id: m_desc
            description: root.artist.description.trim()
            Layout.fillWidth: true
        }
        RowLayout {
            id: m_control_pane
            Layout.alignment: Qt.AlignHCenter

            QA.SortOrderChip {
                Layout.alignment: Qt.AlignVCenter
                model: m_album_sort_type
            }

            Item {
                Layout.fillWidth: true
            }
            MD.StandardIconButton {
                id: m_display_mode_btn
                action: QA.SelectDisplayModeAction {
                    menuParent: m_display_mode_btn
                    displayMode: root.displayMode
                    onSelectDisplayMode: m => root.displayMode = m
                }
            }
            MD.BusyIconButton {
                id: btn_fav
                action: QA.FavoriteAction {
                    itemId: root.itemId
                }
            }
        }
    }

    MD.WidthProvider {
        id: m_wp
        total: m_view.contentWidth
        minimum: 140
        spacing: 12
        leftMargin: 8
        rightMargin: 8
    }

    Component {
        id: dg_albumlist
        QA.ListItemDelegate {
            image: QA.Util.image_url(model.itemId)
            text: model.name
            supportText: {
                const ex = model.extra;
                const tc = model.trackCount;
                const trackInfo = tc > 0 ? qsTr(`${tc} tracks`) : qsTr('no track');
                return [QA.Util.joinName(ex?.artists, '/'), trackInfo].filter(e => !!e).join(' - ');
            }
            onClicked: {
                ListView.view.currentIndex = index;
                QA.Action.route_by_id(model.itemId);
            }
        }
    }

    Component {
        id: dg_album_card
        QA.AlbumCardDelegate {
            widthProvider: m_wp
            mdState.backgroundOpacity: (ListView.view as QA.ItemView).displayMode == QA.Enum.DGrid ? 0 : 1
            hasSubText: true
            subText: QA.Util.formatDateTime(model.publishTime, 'yyyy')
            onClicked: {
                ListView.view.currentIndex = index;
                QA.Action.route_by_id(model.itemId);
            }
        }
    }

    QA.ItemView {
        id: m_view
        anchors.fill: parent

        displayMode: root.displayMode
        topMargin: MD.MProp.size.verticalPadding
        bottomMargin: MD.MProp.size.verticalPadding
        model: qr_artist_albums.data
        delegate: {
            const d = displayMode;
            return [dg_albumlist, dg_album_card, dg_album_card][d];
        }

        header: ColumnLayout {
            id: content
            width: parent.width
            // for control panel
            spacing: 16

            Component.onCompleted: root.viewHeaderItem = this

            MD.Pane {
                id: m_header
                Layout.fillWidth: true
                radius: root.radius
                verticalPadding: 12
                horizontalPadding: 16

                ColumnLayout {
                    width: parent.width

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16
                        visible: !root.single

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
                                Layout.fillWidth: true
                                visible: !!m_desc.description
                                target: m_desc
                            }
                        }
                    }
                    ColumnLayout {
                        spacing: 0
                        Layout.fillWidth: true
                        visible: root.single

                        LayoutItemProxy {
                            Layout.alignment: Qt.AlignHCenter
                            target: m_cover
                        }
                        MD.Space {
                            spacing: 16
                        }
                        ColumnLayout {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.fillWidth: true
                            spacing: 12

                            LayoutItemProxy {
                                Layout.alignment: Qt.AlignHCenter
                                target: m_title
                            }
                            LayoutItemProxy {
                                Layout.alignment: Qt.AlignHCenter
                                target: m_info
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

    QA.AlbumSortTypeModel {
        id: m_album_sort_type
        currentType: QM.AlbumSort.ALBUM_SORT_PUBLISH_TIME
        asc: false
    }
    QA.ArtistQuery {
        id: qr_artist
        Component.onCompleted: reload()
    }
    QA.AlbumsQuery {
        id: qr_artist_albums
        property QM.albumFilter filter1
        filter1.albumArtistIdFilter: {
            const f = QA.Util.albumArtistIdFilter();
            f.value = qr_artist.itemId.id;
            return f;
        }
        property QM.albumFilter filter2
        filter2.artistIdFilter: {
            const f = QA.Util.artistIdFilter();
            f.value = qr_artist.itemId.id;
            return f;
        }
        filters: [root.isAlbumArtistId ? filter1 : filter2]
        asc: m_album_sort_type.asc
        sort: m_album_sort_type.currentType
        onAscChanged: reload()
        onSortChanged: reload()
    }
    Settings {
        id: m_album_setting
        category: "detail.artist.album"
        property alias display_mode: root.displayMode
        property alias sort: m_album_sort_type.currentType
        property alias asc: m_album_sort_type.asc
    }
}

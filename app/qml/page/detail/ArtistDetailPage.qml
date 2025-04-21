pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root

    property var artist: qr_artist.data.item
    property alias itemId: qr_artist.itemId
    readonly property bool single: m_view.width < m_cover.displaySize.width * (1.0 + 1.5) + 8

    title: qsTr("artist")
    padding: 0
    scrolling: !m_view.atYBeginning

    MD.FlickablePane {
        id: m_view_pane
        view: m_view
        excludeBegin: m_view.headerItem.height
        radius: root.radius
        leftMargin: 0
        rightMargin: 0
        x: m_view.leftMargin
        topMargin: MD.MProp.size.verticalPadding
        bottomMargin: 0
    }

    QA.GridView {
        id: m_view
        anchors.fill: parent

        topMargin: MD.MProp.size.verticalPadding
        bottomMargin: MD.MProp.size.verticalPadding
        fixedCellWidth: QA.Util.dyn_card_width(width, spacing)
        model: qr_artist_albums.data

        delegate: QA.PicCardGridDelegate {
            required property var model
            image.source: QA.Util.image_url(model.itemId)
            text: model.name
            subText: QA.Util.formatDateTime(model.publishTime, 'yyyy')
            onClicked: {
                QA.Action.route_by_id(model.itemId);
            }
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
                MD.IconButton {
                    id: btn_fav
                    action: QA.CollectAction {
                        itemId: root.itemId
                    }
                }
            }
        }

        header: ColumnLayout {
            id: content
            width: parent.width
            spacing: 0

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

            MD.Space {
                spacing: MD.MProp.size.verticalPadding * 2
            }
        }
    }
    QA.ArtistQuery {
        id: qr_artist
        Component.onCompleted: reload()
    }
    QA.ArtistAlbumQuery {
        id: qr_artist_albums
        itemId: qr_artist.itemId
        Component.onCompleted: reload()
    }
    // QA.ArtistSongsQuery {
    //     id: qr_artissongs
    //     itemId: qr_artist.itemId
    // }
}

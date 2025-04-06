pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root

    property var artist: qr_artist.data.item
    property alias itemId: qr_artist.itemId
    readonly property bool single: m_flick.width < m_cover.displaySize.width * (1.0 + 1.5) + 8

    title: qsTr("artist")
    padding: 0
    scrolling: !m_flick.atYBeginning

    MD.Flickable {
        id: m_flick
        anchors.fill: parent
        contentHeight: content.implicitHeight
        ScrollBar.vertical.visible: false

        topMargin: MD.MatProp.size.verticalPadding
        bottomMargin: MD.MatProp.size.verticalPadding

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
                Layout.fillWidth: true
                maximumLineCount: 2
                text: root.artist.name
                typescale: MD.Token.typescale.headline_large
            }
            RowLayout {
                id: m_info
                spacing: 12
                MD.Text {
                    typescale: MD.Token.typescale.body_medium
                    text: `${root.artist.albumCount} albums`
                }
                MD.Text {
                    typescale: MD.Token.typescale.body_medium
                    text: `${root.artist.musicCount} songs`
                }
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

        ColumnLayout {
            id: content
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 16

            MD.Pane {
                id: m_header
                Layout.fillWidth: true
                radius: root.radius
                padding: 16

                ColumnLayout {
                    width: parent.width

                    RowLayout {
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
                                Layout.fillWidth: true
                                visible: !!m_desc.description
                                target: m_desc
                            }
                        }
                        MD.Space {
                            spacing: 8
                        }
                    }
                }
            }

            MD.Pane {
                Layout.fillWidth: true
                radius: root.radius

                ColumnLayout {
                    id: pane_view_column
                    width: parent.width
                    spacing: 0

                    LayoutItemProxy {
                        target: m_control_pane
                    }

                    QA.GridView {
                        fixedCellWidth: QA.Util.dyn_card_width(width, spacing)
                        interactive: m_flick.atYEnd
                        implicitHeight: contentHeight
                        // model: qr_artist_albums.data
                        onAtYBeginningChanged: {
                            if (interactive) {
                                m_flick.contentY -= 1;
                            }
                        }

                        delegate: QA.PicCardGridDelegate {
                            required property var model
                            image.source: QA.Util.image_url(model.picUrl)
                            text: model.name
                            subText: Qt.formatDateTime(model.publishTime, 'yyyy')
                            onClicked: {
                                QA.Action.route_by_id(model.itemId);
                            }
                        }
                    }
                }
            }
        }
    }
    QA.ArtistQuery {
        id: qr_artist
        Component.onCompleted: reload()
    }
    // QA.ArtistSongsQuery {
    //     id: qr_artissongs
    //     itemId: qr_artist.itemId
    // }
    // QA.ArtistAlbumsQuery {
    //     id: qr_artist_albums
    //     itemId: qr_artist.itemId
    // }
}

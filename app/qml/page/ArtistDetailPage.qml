import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Page {
    id: root

    property alias itemData: qr_artist.data
    property alias itemId: qr_artist.itemId

    title: qsTr("artist")
    padding: 0
    scrolling: !m_flick.atYBeginning

    MD.Flickable {
        id: m_flick
        anchors.fill: parent
        contentHeight: content.implicitHeight
        ScrollBar.vertical.visible: false

        readonly property bool single: width < m_cover.displaySize.width * (1.0 + 1.5) + 8

        topMargin: MD.MatProp.size.verticalPadding
        bottomMargin: MD.MatProp.size.verticalPadding

        Item {
            visible: false

            QA.Image {
                id: m_cover
                z: 1
                elevation: MD.Token.elevation.level2
                source: QA.Util.image_url(root.itemData.info.picUrl)
                radius: width / 2

                Layout.preferredWidth: displaySize.width
                Layout.preferredHeight: displaySize.height
                displaySize: Qt.size(240, 240)
            }
            MD.Text {
                id: m_title
                Layout.fillWidth: true
                maximumLineCount: 2
                text: root.itemData.info.name
                typescale: MD.Token.typescale.headline_large
            }
            RowLayout {
                id: m_info
                spacing: 12
                MD.Text {
                    typescale: MD.Token.typescale.body_medium
                    text: `${root.itemData.info.albumSize} albums`
                }
                MD.Text {
                    typescale: MD.Token.typescale.body_medium
                    text: `${root.itemData.info.musicSize} songs`
                }
            }
            QA.ListDescription {
                id: m_desc
                description: root.itemData.info.briefDesc.trim()
                Layout.fillWidth: true
            }
            RowLayout {
                id: m_control_pane
                Layout.alignment: Qt.AlignHCenter
                MD.IconButton {
                    id: btn_fav
                    action: QA.SubAction {
                        liked: root.itemData.info.followed
                        querier: qr_sub
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
                        visible: !m_flick.single

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
                        visible: m_flick.single

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

            //RowLayout {
            //    id: ly_header
            //    spacing: 16
            //    Layout.leftMargin: 8
            //    Layout.rightMargin: 8
            //    Layout.topMargin: 8

            //    ColumnLayout {
            //        Layout.alignment: Qt.AlignTop
            //        spacing: 12
            //    }
            //}

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

                    MD.TabBar {
                        id: bar
                        Layout.fillWidth: true

                        Component.onCompleted: {
                            currentIndexChanged();
                        }

                        MD.TabButton {
                            text: qsTr("Hot Song")
                        }
                        MD.TabButton {
                            text: qsTr("Album")
                        }
                    }
                    Item {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        implicitHeight: Math.min(Math.max(root.height - m_header.implicitHeight * 0.4 - bar.implicitHeight, 0), item_stack.implicitHeight)

                        StackLayout {
                            id: item_stack
                            anchors.fill: parent
                            currentIndex: bar.currentIndex

                            MD.ListView {
                                implicitHeight: contentHeight
                                interactive: m_flick.atYEnd
                                model: itemData.hotSongs
                                topMargin: 8
                                bottomMargin: 8
                                leftMargin: 24
                                rightMargin: 24

                                onAtYBeginningChanged: {
                                    if (interactive) {
                                        m_flick.contentY -= 1;
                                    }
                                }

                                delegate: QA.SongDelegate {
                                    subtitle: `${modelData.album.name}`
                                    width: ListView.view.contentWidth

                                    onClicked: {
                                        QA.App.playlist.switchTo(modelData);
                                    }
                                }
                                footer: MD.ListBusyFooter {
                                    running: qr_artist.status === QA.enums.Querying
                                    width: ListView.view.width
                                }
                            }
                            QA.MGridView {
                                fixedCellWidth: Math.max(160, (Window.window?.width ?? 0) / 6.0)
                                interactive: m_flick.atYEnd
                                model: qr_artist_albums.data
                                onAtYBeginningChanged: {
                                    if (interactive) {
                                        m_flick.contentY -= 1;
                                    }
                                }

                                delegate: Item {
                                    width: GridView.view.cellWidth
                                    height: GridView.view.cellHeight
                                    QA.PicGridDelegate {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        anchors.top: parent.top
                                        anchors.topMargin: 8

                                        picWidth: parent.GridView.view.fixedCellWidth
                                        width: picWidth
                                        height: Math.min(implicitHeight, parent.height)
                                        image.source: QA.Util.image_url(model.picUrl)
                                        text: model.name
                                        subText: Qt.formatDateTime(model.publishTime, 'yyyy')

                                        onClicked: {
                                            QA.Global.route(model.itemId);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    QNcm.ArtistQuerier {
        id: qr_artist
        autoReload: itemId.valid()
    }
    QNcm.ArtistSubQuerier {
        id: qr_sub
        autoReload: false
        onStatusChanged: {
            if (status === QA.enums.Finished) {
                QA.App.artistLiked(itemId, sub);
            }
        }
    }
    QNcm.ArtistAlbumsQuerier {
        id: qr_artist_albums
        artistId: qr_artist.itemId
        autoReload: artistId.valid()
    }
}

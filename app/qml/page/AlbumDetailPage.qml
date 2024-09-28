pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Page {
    id: root

    property alias itemData: qr_al.data
    property alias itemId: qr_al.itemId
    title: qsTr("album")

    padding: 0

    MD.ListView {
        id: m_view
        anchors.fill: parent
        reuseItems: true
        contentY: 0

        topMargin: 8
        bottomMargin: 8
        leftMargin: 24
        rightMargin: 24

        model: root.itemData.songs

        readonly property bool single: width < m_cover.displaySize.width * (1.0 + 1.5) + 8

        // listview will takeover the pos

        Item {
            visible: false

            QA.Image {
                id: m_cover
                Layout.preferredWidth: displaySize.width
                Layout.preferredHeight: displaySize.height

                displaySize: Qt.size(240, 240)
                elevation: MD.Token.elevation.level2
                source: `image://ncm/${root.itemData.picUrl}`
                radius: 16
            }
            MD.Text {
                id: m_title
                maximumLineCount: 2
                text: root.itemData.name
                typescale: m_view.single ? MD.Token.typescale.headline_medium : MD.Token.typescale.headline_large
            }
            RowLayout {
                id: m_info
                spacing: 12
                MD.Text {
                    typescale: MD.Token.typescale.body_medium
                    text: `${root.itemData.size} tracks`
                }
                MD.Text {
                    typescale: MD.Token.typescale.body_medium
                    text: Qt.formatDateTime(root.itemData.publishTime, 'yyyy.MM')
                }
            }
            MD.Text {
                id: m_artist
                typescale: MD.Token.typescale.body_medium
                text: QA.Global.join_name(root.itemData.artists, '/')
                /*
                        onClicked: {
                            const artists = root.itemData.artists;
                            if (artists.length === 1)
                                QA.Global.route(artists[0].itemId);
                            else
                                MD.Util.show_popup('qrc:/Qcm/App/qml/component/ArtistsPopup.qml', {
                                        "model": artists
                                    });
                        }
                        */
            }

            QA.ListDescription {
                id: m_desc
                description: root.itemData.description.trim()
            }
        }

        header: Item {
            width: parent.width
            implicitHeight: children[0].implicitHeight

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                RowLayout {
                    spacing: 16
                    visible: !m_view.single

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
                    visible: m_view.single

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
                    MD.Space {
                        spacing: 8
                    }
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    MD.IconButton {
                        action: QA.AppendListAction {
                            getSongs: function () {
                                return root.itemData.songs;
                            }
                        }
                    }
                    MD.IconButton {
                        id: btn_fav
                        action: QA.SubAction {
                            liked: qr_dynamic.data.isSub
                            querier: qr_sub
                            itemId: root.itemId
                        }
                    }
                    MD.IconButton {
                        id: btn_comment
                        action: QNcm.CommentAction {
                            itemId: root.itemId
                        }
                    }
                }
                MD.Space {
                    spacing: 8
                }
            }
        }
        delegate: QA.SongDelegate {
            required property int index
            required property var modelData

            width: ListView.view.contentWidth

            subtitle: QA.Global.join_name(modelData.artists, '/')

            onClicked: {
                QA.App.playlist.switchTo(modelData);
            }
        }

        footer: MD.ListBusyFooter {
            running: qr_al.status === QA.enums.Querying
            width: ListView.view.contentWidth
        }
    }
    MD.FAB {
        flickable: m_view
        action: Action {
            icon.name: MD.Token.icon.play_arrow
            onTriggered: {
                const songs = itemData.songs.filter(s => {
                    return s.canPlay;
                });
                if (songs.length)
                    QA.App.playlist.switchList(songs);
            }
        }
    }

    QNcm.AlbumDetailQuerier {
        id: qr_al
        autoReload: root.itemId.valid()
    }
    QNcm.AlbumDetailDynamicQuerier {
        id: qr_dynamic
        autoReload: itemId.valid()
        itemId: qr_al.itemId
    }
    QNcm.AlbumSubQuerier {
        id: qr_sub
        autoReload: false

        onStatusChanged: {
            if (status === QA.enums.Finished) {
                QA.App.albumLiked(itemId, sub);
            }
        }
    }
}

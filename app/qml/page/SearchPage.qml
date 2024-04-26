import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0

    // MD.MatProp.backgroundColor: item_search.focus ? item_search.MD.MatProp.backgroundColor : MD.Token.color.surface

    function search() {
        this.keywords = item_search.text;
    }

    ColumnLayout {
        anchors.fill: parent
        MD.Pane {
            Layout.fillWidth: true
            padding: 16
            MD.SearchBar {
                id: item_search
                anchors.fill: parent
            }
        }

        MD.TabBar {
            id: bar
            Layout.fillWidth: true

            Component.onCompleted: {
                item_search.accepted.connect(() => {
                        let querier = queriers[currentIndex];
                        if (querier) {
                            querier.keywords = '';
                            querier.keywords = item_search.text;
                        }
                    });
                currentIndexChanged();
            }

            MD.TabButton {
                text: qsTr("Song")
            }
            MD.TabButton {
                text: qsTr("Album")
            }
            MD.TabButton {
                text: qsTr("Playlist")
            }
            MD.TabButton {
                text: qsTr("Djradio")
            }

            readonly property list<QtObject> queriers: [song_querier, album_querier, playlist_querier, djradio_querier]

            onCurrentIndexChanged: {
                let querier = queriers[currentIndex];
                if (querier && querier.keywords != item_search.text) {
                    querier.keywords = item_search.text;
                }
            }
        }

        MD.Pane {
            Layout.fillHeight: true
            Layout.fillWidth: true
            padding: 0

            MD.MatProp.backgroundColor: MD.Token.color.surface

            StackLayout {
                id: item_stack
                anchors.fill: parent
                currentIndex: bar.currentIndex

                MD.ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: song_querier.data
                    busy: song_querier.status === QA.qcm.Querying

                    delegate: QA.SongDelegate {
                        width: ListView.view.width
                        model_: QA.App.song(model)
                        onClicked: {
                            QA.Global.playlist.switchTo(model_);
                        }
                    }

                    QA.CloudSearchQuerier {
                        id: song_querier
                        autoReload: keywords
                        type: QA.CloudSearchQuerier.SongType
                    }
                }

                MD.ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: album_querier.data
                    busy: album_querier.status === QA.qcm.Querying

                    delegate: MD.ListItem {
                        property var itemId: model.itemId

                        width: ListView.view.width
                        text: model.name
                        maximumLineCount: 2
                        supportText: `${QA.Global.join_name(model.artists, '/')} - ${Qt.formatDateTime(model.publishTime, 'yyyy.M.d')} - ${model.trackCount} tracks`
                        leader: MD.Image {
                            radius: 8
                            source: `image://ncm/${model.picUrl}`
                            sourceSize.height: 48
                            sourceSize.width: 48
                        }
                        onClicked: {
                            QA.Global.route(itemId);
                            ListView.view.currentIndex = index;
                        }
                    }

                    QA.CloudSearchQuerier {
                        id: album_querier
                        autoReload: keywords
                        type: QA.CloudSearchQuerier.AlbumType
                    }
                }

                MD.ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: playlist_querier.data
                    busy: playlist_querier.status === QA.qcm.Querying

                    delegate: MD.ListItem {
                        property var itemId: model.itemId

                        width: ListView.view.width
                        text: model.name
                        maximumLineCount: 2
                        supportText: `${model.trackCount} songs`
                        leader: MD.Image {
                            radius: 8
                            source: `image://ncm/${model.picUrl}`
                            sourceSize.height: 48
                            sourceSize.width: 48
                        }
                        onClicked: {
                            QA.Global.route(itemId);
                            ListView.view.currentIndex = index;
                        }
                    }

                    QA.CloudSearchQuerier {
                        id: playlist_querier
                        autoReload: keywords
                        type: QA.CloudSearchQuerier.PlaylistType
                    }
                }
                MD.ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: djradio_querier.data
                    busy: djradio_querier.status === QA.qcm.Querying

                    delegate: MD.ListItem {
                        property var itemId: model.itemId

                        width: ListView.view.width
                        text: model.name
                        maximumLineCount: 2
                        supportText: `${model.programCount} programs`
                        leader: MD.Image {
                            radius: 8
                            source: `image://ncm/${model.picUrl}`
                            sourceSize.height: 48
                            sourceSize.width: 48
                        }
                        onClicked: {
                            QA.Global.route(itemId);
                            ListView.view.currentIndex = index;
                        }
                    }

                    QA.CloudSearchQuerier {
                        id: djradio_querier
                        autoReload: keywords
                        type: QA.CloudSearchQuerier.DjradioType
                    }
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNCM
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0
    title: qsTr('search')

    // MD.MatProp.backgroundColor: item_search.focus ? item_search.MD.MatProp.backgroundColor : MD.Token.color.surface

    function search() {
        this.keywords = item_search.text;
    }

    component BaseView: MD.ListView {
        implicitHeight: contentHeight
        model: querier.data
        busy: querier.status === QA.enums.Querying
        leftMargin: 24
        rightMargin: 24

        property alias querier: querier
        property alias type: querier.type

        QNCM.CloudSearchQuerier {
            id: querier
            autoReload: keywords
        }
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

            function get_querier() {
                return m_stack.children[currentIndex]?.querier;
            }

            Component.onCompleted: {
                item_search.accepted.connect(() => {
                    let querier = get_querier();
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

            onCurrentIndexChanged: {
                let querier = get_querier();
                if (querier && querier.keywords != item_search.text) {
                    querier.keywords = item_search.text;
                }
            }
        }

        MD.Pane {
            Layout.fillHeight: true
            Layout.fillWidth: true
            padding: 0

            backgroundColor: MD.Token.color.surface

            StackLayout {
                id: m_stack
                anchors.fill: parent
                currentIndex: bar.currentIndex

                BaseView {
                    delegate: QA.SongDelegate {
                        width: ListView.view.contentWidth
                        dgModel: QA.Util.create_song(model)
                        onClicked: {
                            QA.App.playlist.switchTo(dgModel);
                        }
                    }
                    type: QNCM.CloudSearchQuerier.SongType
                }

                BaseView {
                    delegate: MD.ListItem {
                        width: ListView.view.contentWidth
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
                            QA.Global.route(model.itemId);
                            ListView.view.currentIndex = index;
                        }
                    }

                    type: QNCM.CloudSearchQuerier.AlbumType
                }

                BaseView {
                    implicitHeight: contentHeight
                    delegate: MD.ListItem {
                        width: ListView.view.contentWidth
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
                            QA.Global.route(model.itemId);
                            ListView.view.currentIndex = index;
                        }
                    }
                    type: QNCM.CloudSearchQuerier.PlaylistType
                }
                BaseView {
                    delegate: MD.ListItem {
                        width: ListView.view.contentWidth
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
                            QA.Global.route(model.itemId);
                            ListView.view.currentIndex = index;
                        }
                    }

                    type: QNCM.CloudSearchQuerier.DjradioType
                }
            }
        }
    }
}

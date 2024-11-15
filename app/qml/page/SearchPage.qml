import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as QC
import Qcm.App as QA
import Qcm.Service.Ncm as QNCM
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0
    title: qsTr('search')
    topPadding: showHeader ? 0 : MD.MatProp.size.verticalPadding

    function search() {
        this.keywords = item_search.text;
    }

    component BaseView: MD.ListView {
        implicitHeight: contentHeight
        model: querier.data
        busy: querier.status === QA.enums.Querying

        topMargin: 8
        bottomMargin: MD.MatProp.size.verticalPadding + m_view_pane.bottomMargin

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
        spacing: 16

        MD.SearchBar {
            id: item_search
            Layout.fillWidth: true
        }

        RowLayout {
            MD.FilterChip {
                action: QC.Action {
                    text: qsTr('local')
                    icon.name: MD.Token.icon.arrow_drop_down
                    onTriggered: {}
                }
            }
            MD.FilterChip {
                text: qsTr('song')
            }
            MD.FilterChip {
                text: qsTr('album')
            }
            MD.FilterChip {
                text: qsTr('artist')
            }
        }

        ColumnLayout {
            spacing: 0
            MD.TabBar {
                id: bar
                Layout.fillWidth: true
                corners: MD.Util.corner(root.radius, 0)

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

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true

                MD.FlickablePane {
                    id: m_view_pane
                    view: m_stack.children[m_stack.currentIndex]
                    corners: MD.Util.corner(0, root.radius)
                    color: root.MD.MatProp.backgroundColor
                }

                StackLayout {
                    id: m_stack
                    anchors.fill: parent
                    currentIndex: bar.currentIndex

                    BaseView {
                        delegate: QA.SongDelegate {
                            width: ListView.view.contentWidth
                            onClicked: {
                                QA.Action.play_by_id(dgModel.itemId);
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
                                source: QA.Util.image_url(model.picUrl)
                                sourceSize.height: 48
                                sourceSize.width: 48
                            }
                            onClicked: {
                                QA.Action.route_by_id(model.itemId);
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
                                source: QA.Util.image_url(model.picUrl)
                                sourceSize.height: 48
                                sourceSize.width: 48
                            }
                            onClicked: {
                                QA.Action.route_by_id(model.itemId);
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
                                source: QA.Util.image_url(model.picUrl)
                                sourceSize.height: 48
                                sourceSize.width: 48
                            }
                            onClicked: {
                                QA.Action.route_by_id(model.itemId);
                                ListView.view.currentIndex = index;
                            }
                        }

                        type: QNCM.CloudSearchQuerier.DjradioType
                    }
                }
            }
        }
    }
}

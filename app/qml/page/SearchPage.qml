pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as QC
import Qcm.App as QA
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
        model: m_query.data
        busy: m_query.status === QA.enums.Querying

        topMargin: 8
        bottomMargin: MD.MatProp.size.verticalPadding * 2

        leftMargin: 24
        rightMargin: 24

        property alias query: m_query
        property alias type: m_query.type

        QA.SearchQuery {
            id: m_query
        }
    }

    QA.SearchTypeModel {
        id: m_search_type_model
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

            MD.ButtonGroup {
                id: m_group
            }

            Repeater {
                model: m_search_type_model
                MD.FilterChip {
                    required property var model
                    text: model.name
                    MD.ButtonGroup.group: m_group

                    onClicked: {
                        m_search_type_model.currentIndex = model.index;
                    }
                }
            }

            Component.onCompleted: {
                m_group.buttons[m_search_type_model.currentIndex].checked = true;
            }
        }

        ColumnLayout {
            spacing: 0

            /*
            MD.TabBar {
                id: bar
                Layout.fillWidth: true
                corners: MD.Util.corner(root.radius, 0)

                function get_query() {
                    return m_stack.children[currentIndex]?.query;
                }

                Component.onCompleted: {
                    item_search.accepted.connect(() => {
                        let query = get_query();
                        if (query) {
                            query.keywords = '';
                            query.keywords = item_search.text;
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
                    let query = get_query();
                    if (query && query.keywords != item_search.text) {
                        query.keywords = item_search.text;
                    }
                }
            }
            */

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true

                //MD.FlickablePane {
                //    id: m_view_pane
                //    view: m_stack.children[m_stack.currentIndex]
                //    corners: MD.Util.corner(0, root.radius)
                //    color: root.MD.MatProp.backgroundColor
                //}

                StackLayout {
                    id: m_stack
                    anchors.fill: parent
                    currentIndex: m_search_type_model.currentIndex
                    property list<var> delegates: [m_dg_song, m_dg_album, m_dg_mix, m_dg_radio]

                    Repeater {
                        model: m_search_type_model

                        BaseView {
                            required property var model
                            delegate: m_stack.delegates[m_stack.currentIndex]
                            type: model.type
                        }
                    }

                    Component {
                        id: m_dg_song
                        QA.SongDelegate {
                            required property var model
                            width: ListView.view.contentWidth
                            onClicked: {
                                QA.Action.play_by_id(dgModel.itemId);
                            }
                        }
                    }
                    Component {
                        id: m_dg_album
                        MD.ListItem {
                            required property var model
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
                    }

                    Component {
                        id: m_dg_mix
                        MD.ListItem {
                            required property var model
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
                    }

                    Component {
                        id: m_dg_radio
                        MD.ListItem {
                            required property var model
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
                    }
                }
            }
        }
    }
}

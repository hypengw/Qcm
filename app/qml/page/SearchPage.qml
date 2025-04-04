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

    component BaseView: MD.ListView {
        implicitHeight: contentHeight
        model: m_query.data
        busy: m_query.status === QA.Enum.Querying

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
            id: m_search_bar
            Layout.fillWidth: true
            onAccepted: {
                let query = m_stack.get_query();
                if (query) {
                    query.text = m_search_bar.text;
                    query.reload();
                }
            }
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
                            delegate: m_stack.delegates[model.index]
                            type: model.type
                        }
                    }

                    function get_query() {
                        return m_stack.children[currentIndex]?.query;
                    }

                    onCurrentIndexChanged: {
                        let query = get_query();
                        if (query && query.text != m_search_bar.text) {
                            query.text = m_search_bar.text;
                            query.reload();
                        }
                    }

                    Component {
                        id: m_dg_song
                        QA.SongDelegate {
                            required property int index
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
                            id: m_item
                            required property int index
                            required property var model
                            width: ListView.view.contentWidth
                            rightPadding: 0
                            text: model.name
                            maximumLineCount: 2
                            supportText: [model.artists.map(el => el.name).join('/'), Qt.formatDateTime(model.publishTime, 'yyyy')].filter(Boolean).join(' • ')
                            leader: QA.Image {
                                radius: 8
                                source: QA.Util.image_url(m_item.model.picUrl)
                                sourceSize.height: 48
                                sourceSize.width: 48
                            }
                            trailing: MD.IconButton {
                                icon.name: MD.Token.icon.more_vert

                                onClicked: {
                                    const props = {
                                        "itemId": m_item.model.itemId,
                                        "y": height
                                    };
                                    MD.Util.show_popup('qrc:/Qcm/App/qml/menu/AlbumMenu.qml', props, this);
                                }
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
                            leader: QA.Image {
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
                            leader: QA.Image {
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

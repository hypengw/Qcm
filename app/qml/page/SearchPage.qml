pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0
    title: qsTr('search')
    topPadding: showHeader ? 0 : MD.MProp.size.verticalPadding

    property list<var> models: [m_model.songs, m_model.albums, m_model.artists]
    property list<var> delegates: [m_dg_song, m_dg_album, m_dg_artist]

    QA.SearchModel {
        id: m_model
    }

    QA.SearchTypeModel {
        id: m_search_type_model
    }

    component BaseView: MD.VerticalListView {
        property string text
        readonly property alias query: m_query

        implicitHeight: contentHeight
        busy: query.status === QA.Enum.Querying
        topMargin: 8
        bottomMargin: MD.MProp.size.verticalPadding * 2

        leftMargin: 12
        rightMargin: 12

        QA.SearchQuery {
            id: m_query
            data: m_model
            function doQuery(text: string, type: int) {
                const q = m_query;
                q.text = text;
                q.type = type;
                q.reload();
            }
        }
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        MD.SearchBar {
            id: m_search_bar
            Layout.fillWidth: true
            onAccepted: {
                m_stack.updateText(text);
            }
        }

        RowLayout {
            MD.FilterChip {
                action: MD.Action {
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
                //    color: root.MD.MProp.backgroundColor
                //}

                StackLayout {
                    id: m_stack
                    anchors.fill: parent
                    currentIndex: m_search_type_model.currentIndex

                    Repeater {
                        model: m_search_type_model

                        BaseView {
                            required property int index
                            model: root.models[index]
                            delegate: root.delegates[index]
                        }
                    }

                    function updateText(text: string) {
                        const q = m_stack.children[currentIndex]?.query;
                        if (!q) {
                            return;
                        }
                        const v = m_stack.children[currentIndex];
                        if (v && v.text != text) {
                            v.text = text;
                            q.doQuery(text, m_search_type_model.currentType);
                        }
                    }

                    onCurrentIndexChanged: {
                        updateText(m_search_bar.text);
                    }

                    Component {
                        id: m_dg_song
                        QA.SongDelegate {
                            required property int index
                            required property var model
                            width: ListView.view.contentWidth
                            mdState.backgroundColor: mdState.ctx.color.surface
                            useTracknumber: false
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
                            corners: indexCorners(index, count, 16)
                            supportText: {
                                const ex = QA.Store.extra(model.itemId);
                                return [QA.Util.joinName(ex?.artists), QA.Util.formatDateTime(model.publishTime, 'yyyy')].filter(e => !!e).join(' • ');
                            }
                            leader: QA.Image {
                                radius: 8
                                source: QA.Util.image_url(model.itemId)
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
                                    MD.Util.showPopup('qrc:/Qcm/App/qml/menu/AlbumMenu.qml', props, this);
                                }
                            }
                            onClicked: {
                                QA.Action.route_by_id(model.itemId);
                                ListView.view.currentIndex = index;
                            }
                        }
                    }

                    Component {
                        id: m_dg_artist
                        MD.ListItem {
                            required property int index
                            required property var model
                            width: ListView.view.contentWidth
                            text: model.name
                            maximumLineCount: 2
                            corners: indexCorners(index, count, 16)
                            supportText: `${model.albumCount} albums`
                            leader: QA.Image {
                                radius: 8
                                source: QA.Util.image_url(model.itemId)
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

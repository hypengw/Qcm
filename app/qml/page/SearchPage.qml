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

    property list<QtObject> models: [m_model.songs, m_model.albums, m_model.artists]
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
        spacing: 8

        ColumnLayout {
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            spacing: 16

            MD.SearchBar {
                id: m_search_bar
                Layout.fillWidth: true

                onAccepted: {
                    m_stack.updateText(text);
                }
            }

            Row {
                spacing: 4
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
                            m_stack.currentIndex = model.index;
                            m_stack.updateText(m_search_bar.text);
                        }

                        Component.onCompleted: {
                            checked = m_search_type_model.currentIndex == model.index;
                        }
                    }
                }
            }
        }

        ColumnLayout {
            spacing: 0

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true

                //MD.FlickablePane {
                //    id: m_view_pane
                //    view: m_stack.children[m_stack.currentIndex]
                //    corners: MD.Util.corners(0, root.radius)
                //    color: root.MD.MProp.backgroundColor
                //}

                StackLayout {
                    id: m_stack
                    anchors.fill: parent

                    Repeater {
                        id: m_view_repeater
                        model: m_search_type_model
                        BaseView {
                            required property int index
                            model: root.models[index]
                            delegate: root.delegates[index]
                            Component.onCompleted: {
                                // workaround for async and qt stacklayout > 6.5
                                m_stack.currentIndex = 0;
                            }
                        }
                    }
                    function updateText(text: string) {
                        const r = m_view_repeater;
                        const q = r.itemAt(currentIndex)?.query;
                        if (!q) {
                            return;
                        }
                        const v = r.itemAt(currentIndex);
                        if (v && v.text != text) {
                            v.text = text;
                            q.doQuery(text, m_search_type_model.currentType);
                        }
                    }
                    Component {
                        id: m_dg_song
                        QA.SongDelegate {
                            width: ListView.view.contentWidth
                            mdState.backgroundColor: mdState.ctx.color.surface
                            useTracknumber: false
                            onClicked: {
                                QA.Action.play(dgModel.itemId);
                            }
                        }
                    }
                    Component {
                        id: m_dg_album
                        MD.ListItem {
                            id: m_item
                            width: ListView.view.contentWidth
                            rightPadding: 0
                            text: model.name
                            maximumLineCount: 2
                            corners: MD.Util.listCorners(index, count, 16)
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
                                QA.Action.routeItem(model.itemId);
                                ListView.view.currentIndex = index;
                            }
                        }
                    }

                    Component {
                        id: m_dg_artist
                        MD.ListItem {
                            width: ListView.view.contentWidth
                            text: model.name
                            maximumLineCount: 2
                            corners: MD.Util.listCorners(index, count, 16)
                            supportText: `${model.albumCount} albums`
                            leader: QA.Image {
                                radius: 8
                                source: QA.Util.image_url(model.itemId)
                                sourceSize.height: 48
                                sourceSize.width: 48
                            }
                            onClicked: {
                                QA.Action.routeItem(model.itemId);
                                ListView.view.currentIndex = index;
                            }
                        }
                    }
                }
            }
        }
    }
}

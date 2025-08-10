pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Msg as QM
import Qcm.Material as MD

MD.ItemDelegate {
    id: root
    required property var model
    required property int index

    font.capitalization: Font.MixedCase

    function openMenu() {
        QA.Action.openPopup(m_menu_comp);
    }

    contentItem: RowLayout {
        Row {
            id: m_loader_row
            Layout.fillWidth: true
            spacing: 0
            MD.Loader {
                id: m_loader
                width: parent.width

                property string filterUrl: {
                    const base = 'qrc:/Qcm/App/qml/component/filter/';
                    switch (root.model.type) {
                    case QM.FilterType.FILTER_TYPE_TITLE:
                        return base + 'TitleFilter.qml';
                    case QM.FilterType.FILTER_TYPE_TRACK_COUNT:
                        return base + 'TrackFilter.qml';
                    case QM.FilterType.FILTER_TYPE_ARTIST_NAME:
                        return base + 'ArtistNameFilter.qml';
                    default:
                        return '';
                    }
                }

                Component.onCompleted: {
                    reload();
                    filterUrlChanged.connect(reload);
                }
                function reload() {
                    if (filterUrl) {
                        setSource(m_loader.filterUrl, {
                            filter: root.model
                        });
                    } else {
                        sourceComponent = null;
                        sourceComponent = m_empty_comp;
                    }
                }

                Connections {
                    function onClicked() {
                        root.openMenu();
                    }
                    target: m_loader.item
                    ignoreUnknownSignals: true
                }
            }
        }

        MD.SmallIconButton {
            id: m_act
            icon.name: MD.Token.icon.close
            onClicked: {
                const v = root.ListView.view;
                v.model.removeRow(root.index);
            }
        }
    }

    background: MD.Rectangle {
        corners: {
            const v = root.ListView.view;
            return MD.Util.listCorners(root.index, v.count, 12);
        }
        color: root.MD.MProp.color.surface
    }

    Component {
        id: m_empty_comp
        Flow {
            MD.InputChip {
                text: qsTr('empty')
                onClicked: {
                    root.openMenu();
                }  
            }
        }
    }

    Component {
        id: m_menu_comp
        QA.AlbumFilterMenu {
            parent: root
            y: root.contentItem.y + root.contentItem.height
            onSelected: type => {
                root.model.type = type;
                close();
            }
        }
    }
}

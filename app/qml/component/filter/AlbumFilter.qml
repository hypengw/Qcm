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
                width: parent.width
                sourceComponent: {
                    switch (root.model.type) {
                    case QM.FilterType.FILTER_TYPE_TITLE:
                        return m_title_comp;
                    case QM.FilterType.FILTER_TYPE_TRACK_COUNT:
                        return m_track_comp;
                    default:
                        return m_empty_comp;
                    }
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
        id: m_title_comp
        QA.StringFilter {
            name: qsTr('title')
            onClicked: root.openMenu()
            property QM.titleFilter filter
            function doCommit() {
                toFilter(filter);
                root.model.titleFilter = filter;
            }
            Component.onCompleted: {
                if (!root.model.hasTitleFilter) {
                    root.model.titleFilter = filter;
                }
                fromFilter(root.model.titleFilter);
                commit.connect(doCommit);
            }
        }
    }

    Component {
        id: m_track_comp
        QA.IntFilter {
            name: qsTr('track')
            onClicked: root.openMenu()
            property QM.trackCountFilter filter
            function doCommit() {
                toFilter(filter);
                root.model.trackFilter = filter;
            }
            Component.onCompleted: {
                if (!root.model.hasTrackFilter) {
                    root.model.trackFilter = filter;
                }
                fromFilter(root.model.trackFilter);
                commit.connect(doCommit);
            }
        }
    }

    Component {
        id: m_empty_comp
        Flow {
            MD.InputChip {
                text: qsTr('empty')
                onClicked: root.openMenu()
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

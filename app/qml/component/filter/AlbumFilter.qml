pragma ComponentBehavior: Bound
import QtQuick

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

    contentItem: Row {
        Row {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - m_act.width
            spacing: 0
            MD.Loader {
                sourceComponent: {
                    switch (root.model.type) {
                    case QM.FilterType.FILTER_TYPE_TITLE:
                        return m_title_comp;
                    default:
                        return m_empty_comp;
                    }
                }
            }
        }

        MD.SmallIconButton {
            id: m_act
            anchors.verticalCenter: parent.verticalCenter
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
            onClicked: root.openMenu()
            property QM.titleFilter filter
            onFilterChanged: {
                filter.value = value;
                filter.condition = condition;
                root.model.titleFilter = filter;
            }
            Component.onCompleted: {
                if (!root.model.hasTitleFilter) {
                    root.model.titleFilter = filter;
                } else {
                    filter = root.model.titleFilter;
                }
            }
        }
    }

    Component {
        id: m_empty_comp
        MD.InputChip {
            text: qsTr('empty')
            onClicked: root.openMenu()
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

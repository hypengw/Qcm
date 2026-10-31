pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Msg as QM
import Qcm.Material as MD

MD.ItemDelegate {
    id: root
    required property MD.LazyRow row
    required property QA.FilterRuleModel ruleModel
    required property MD.ItemModelSource itemSource
    readonly property int index: row.index
    readonly property var model: {
        void row.revision;
        return itemSource.objectForKey(row.key);
    }

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
                    switch (root.model?.type) {
                    case QM.FilterType.FILTER_TYPE_NAME:
                        return base + 'NameFilter.qml';
                    case QM.FilterType.FILTER_TYPE_YEAR:
                        return base + 'YearFilter.qml';
                    case QM.FilterType.FILTER_TYPE_ALBUM_TITLE:
                        return base + 'AlbumTitleFilter.qml';
                    default:
                        return base + 'EmptyFilter.qml';
                    }
                }

                Component.onCompleted: {
                    reload();
                    filterUrlChanged.connect(reload);
                    root.modelChanged.connect(reload);
                }
                function reload() {
                    source = "";
                    if (!root.model) return;
                    setSource(m_loader.filterUrl, {
                        filter: root.model
                    });
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

        MD.StandardIconButton {
            icon.width: 22
            icon.height: 22
            implicitBackgroundSize: 0
            id: m_act
            icon.name: MD.Token.icon.close
            onClicked: {
                root.ruleModel.removeRow(root.index);
            }
        }
    }

    background: MD.Rectangle {
        corners: {
            const m = root.ruleModel;
            void(root.row.revision);
            return MD.Util.listCorners(m.rowIndexInGroup(root.index),
                                       m.rowCountInGroupOf(root.index), 12);
        }
        color: root.MD.MProp.color.surface
    }

    Component {
        id: m_menu_comp
        QA.ArtistFilterMenu {
            parent: root
            y: root.contentItem.y + root.contentItem.height
            onSelected: type => {
                root.model.type = type;
                close();
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qt.labs.settings
import QcmApp
import ".."
import "../component"
import "../part"

MPage {
    id: root

    property list<string> mine_cats: []

    padding: 0
    title: qsTr('playlist tags')

    MFlickable {
        id: flick
        anchors.fill: parent
        leftMargin: 4
        rightMargin: 4

        ScrollBar.vertical: ScrollBar {
        }

        ColumnLayout {
            id: content
            height: implicitHeight
            spacing: 12

            anchors {
                left: parent.left
                leftMargin: 12
                right: parent.right
                rightMargin: 12
            }
            CatalogueSection {
                id: mine_section

                property var set: new Set(settings.value('cat_list', []))

                function add(cat) {
                    if (set.has(set))
                        return;
                    set.add(cat);
                    setChanged();
                    root.mine_cats = root.mine_cats.concat([cat]);
                }
                function remove(cat) {
                    if (!set.has(cat))
                        return;
                    set.delete(cat);
                    setChanged();
                    root.mine_cats = root.mine_cats.filter(el => {
                            return set.has(el);
                        });
                }

                is_mine: true
                model: root.mine_cats.map(s => {
                        return {
                            "name": s
                        };
                    })
                text: 'Mine'
            }
            Repeater {
                model: qr_cat.data.categories

                delegate: CatalogueSection {
                    model: qr_cat.data.category(modelData)
                    text: modelData
                }
            }
        }
    }
    Settings {
        id: settings

        property alias cat_list: root.mine_cats

        category: QA.user_setting_category
    }
    ApiContainer {
        PlaylistCatalogueQuerier {
            id: qr_cat
        }
    }

    component CatalogueSection: ColumnLayout {
        id: cat_section

        property bool is_mine: false
        property alias model: flow_repeater.model
        property alias text: label.text

        Label {
            id: label
            font.pointSize: Theme.ts.label_large.size
        }
        Flow {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                id: flow_repeater
                delegate: MButton {
                    Material.accent: Theme.color.surface_container_lowest
                    Material.elevation: 0
                    enabled: cat_section.is_mine || !mine_section.set.has(modelData.name)
                    highlighted: true

                    action: Action {
                        text: modelData.name

                        onTriggered: {
                            if (cat_section.is_mine)
                                mine_section.remove(modelData.name);
                            else
                                mine_section.add(modelData.name);
                        }
                    }
                }
            }
        }
    }
}

pragma ComponentBehavior: Bound
import QtQuick
import Qcm.App as QA 
import Qcm.Material as MD

Flow {
    id: root
    spacing: 12

    property string name
    property int condition

    signal clicked
    signal commit

    Connections {
        target: root
        ignoreUnknownSignals: true
        function onConditionChanged() {
            root.commit();
        }
        function onValueChanged() {
            root.commit();
        }
    }

    property alias nameChip: m_name
    property var conditionModel
    MD.InputChip {
        id: m_name
        text: root.name
        onClicked: root.clicked()
    }
    MD.InputChip {
        id: m_condition
        readonly property alias model: root.conditionModel
        text: {
            const item = model.find(m => m.value === root.condition);
            return item.name;
        }
        onClicked: QA.Action.openPopup(m_menu_comp)
        Component {
            id: m_menu_comp
            MD.Menu {
                id: m_menu
                parent: m_condition
                y: parent.height
                model: m_condition.model
                contentDelegate: MD.MenuItem {
                    required property var model
                    text: model.name
                    onClicked: {
                        root.condition = model.value;
                        m_menu.close();
                    }
                }
            }
        }
    }
}

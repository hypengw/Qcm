import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD
import Qcm.App as QA

MD.Pane {
    id: root
    default property alias columnData: m_column.data
    property alias title: m_title.text
    padding: 0
    font.capitalization: Font.Capitalize

    ColumnLayout {
        id: m_column
        anchors.fill: parent
        spacing: root.spacing

        MD.Label {
            id: m_title
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            MD.MProp.textColor: MD.Token.color.primary
            typescale: MD.Token.typescale.title_medium
            visible: text
        }

        Component.onCompleted: {
            let start = m_column.children[1];
            if (start instanceof QA.SettingRow) {
                start.start = true;
            }
            let end = m_column.children[m_column.children.length - 1];
            if (end instanceof QA.SettingRow) {
                end.end = true;
            }
        }
    }
}

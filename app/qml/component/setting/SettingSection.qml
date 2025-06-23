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

    signal columnChanged
    onColumnChanged: {
        if (!visible)
            return;
        const children = m_column.children.filter(el => {
            const ok = el instanceof QA.SettingRow;
            if (ok) {
                el.start = false;
                el.end = false;
            }
            return ok && el.visible;
        });
        let start = children[0];
        if (start)
            start.start = true;
        let end = children[children.length - 1];
        if (end)
            end.end = true;
    }
    Component.onCompleted: {
        visibleChanged.connect(columnChanged);
        columnChanged();
    }

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
    }
}

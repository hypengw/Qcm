pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.App as QA
import Qcm.Material as MD

MD.Dialog {
    id: root
    title: qsTr('filter')
    property QA.AlbumFilterRuleModel model
    horizontalPadding: 16

    standardButtons: T.Dialog.Apply | T.Dialog.Reset
    onApplied: {
        model.apply();
        accept();
    }
    onReset: {}

    Component.onCompleted: {
        let btn = standardButton(T.Dialog.Reset);
        btn.enabled = Qt.binding(() => model.dirty);
        btn = standardButton(T.Dialog.Apply);
        btn.enabled = Qt.binding(() => model.dirty);
    }

    contentItem: ColumnLayout {
        RowLayout {
            MD.Label {
                Layout.fillWidth: true
                text: qsTr('Rule')
                typescale: MD.Token.typescale.title_medium
            }
            Row {
                spacing: 0
                MD.IconButton {
                    icon.name: MD.Token.icon.clear_all
                    onClicked: {
                        root.model.removeRows(0, root.model.rowCount());
                    }
                }
                MD.IconButton {
                    icon.name: MD.Token.icon.add
                    onClicked: {
                        root.model.insertRow(-1);
                    }
                }
            }
        }

        MD.VerticalListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: -16
            Layout.rightMargin: -16

            model: root.model
            delegate: QA.AlbumFilter {
                width: ListView.view.contentWidth
            }
            implicitHeight: contentHeight
            spacing: 2
            leftMargin: 16
            rightMargin: 16
        }
    }
}

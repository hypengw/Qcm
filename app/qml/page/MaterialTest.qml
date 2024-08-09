import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QC

import Qcm.Material as MD
import Qcm.App as QA

MD.Page {
    padding: 8

    contentItem: ColumnLayout {
        id: content
        spacing: 0

        MD.TabBar {
            id: bar_test
            Layout.fillWidth: true
            spacing: 0
            clip: true

            MD.TabButton {
                action: QC.Action {
                    text: 'color'
                    onTriggered: {
                        bar_test.currentIndex = 0;
                        view_container.switchTo('qrc:/Qcm/Material/Example/Color.qml', {});
                    }
                }

                Component.onCompleted: {
                    action.trigger();
                }
            }
            MD.TabButton {
                action: QC.Action {
                    text: 'shadow'
                    onTriggered: {
                        bar_test.currentIndex = 1;
                        view_container.switchTo('qrc:/Qcm/Material/Example/Shadow.qml', {});
                    }
                }
            }
            MD.TabButton {
                action: QC.Action {
                    text: 'typography'
                    onTriggered: {
                        bar_test.currentIndex = 2;
                        view_container.switchTo('qrc:/Qcm/Material/Example/Typography.qml', {});
                    }
                }
            }
        }

        MD.Pane {
            Layout.fillHeight: true
            Layout.fillWidth: true
            padding: 0

            QA.PageContainer {
                id: view_container
                anchors.fill: parent
                initialItem: Item {}
            }
        }
    }
}

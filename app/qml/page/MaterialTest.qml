import QtQuick
import QtQuick.Layouts

import Qcm.Material as MD
import org.kde.kirigami as Kirigami

MD.Page {
    padding: 16

    ColumnLayout {
        anchors.fill: parent

        MD.Pane {
            Layout.fillWidth: true
            MD.MatProp.elevation: MD.Token.elevation.level2
            padding: 0

            radius: 16

            ColumnLayout {
                anchors.fill: parent

                spacing: 0
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 48
                    color: MD.Token.color.background

                    MD.Text {
                        anchors.centerIn: parent
                        text: 'background'
                    }
                }
                MD.Divider {}

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 48
                    color: MD.Token.color.surface
                    MD.Text {
                        anchors.centerIn: parent
                        text: 'surface'
                    }
                }
            }
        }

        RowLayout {

        Kirigami.ShadowedRectangle {
            width: 100
            height: 100
            radius: 50
            color: MD.Token.color.primary
        }
        Rectangle {
            width: 100
            height: 100
            radius: 50
            color: MD.Token.color.primary
        }
        }
    }
}

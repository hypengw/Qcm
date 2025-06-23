import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

Item {
    id: root
    property alias corners: m_round.corners

    MD.Shape {
        anchors.fill: parent
        MD.RoundPath {
            id: m_round
            strokeColor: "transparent"
            width: root.width
            height: root.height
            fillGradient: LinearGradient {
                x1: 0
                y1: 0
                x2: 0
                y2: root.height
                GradientStop {
                    position: 0
                    color: Qt.alpha("#000000", 0)
                }
                GradientStop {
                    position: 0.5
                    color: Qt.alpha("#000000", 0.25)
                }
                GradientStop {
                    position: 1
                    color: Qt.alpha("#000000", 0.75)
                }
            }
        }
    }
}

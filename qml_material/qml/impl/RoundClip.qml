import QtQuick
import Qcm.Material as MD

ShaderEffect {
    property var source
    property var radius: 0
    property vector4d radius_: MD.Util.corner(radius).toVector4D()
    property real size: 1
    property real smoothing: 0.8
    fragmentShader: 'qrc:/Qcm/Material/assets/shader/round.frag.qsb'
}

import QtQuick

Item {
    id: root

    property vector2d lower
    property vector2d higher
    property real sigma
    property real corner
    property color color: "white"

    ShaderEffect {
        id: shaderItem

        x: (parent.width - width) / 2.0
        y: (parent.height - height) / 2.0
        width: parent.width
        height: parent.height

        property vector2d lower: root.lower
        property vector2d higher: root.higher
        property real sigma: root.sigma
        property real corner: root.corner
        property color color: root.color

        fragmentShader: 'qrc:/Qcm/Material/assets/shader/shadow.frag.qsb'
    }
}

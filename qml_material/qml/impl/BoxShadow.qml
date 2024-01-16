import QtQuick

Item {
    id: rootItem

    property real blurRadius: 0.0
    property real spreadRadius: 0.0
    property color color: "white"

    ShaderEffect {
        id: shaderItem

        x: (parent.width - width) / 2.0
        y: (parent.height - height) / 2.0
        width: parent.width + rootItem.glowRadius * 2 + cornerRadius * 2
        height: parent.height + rootItem.glowRadius * 2 + cornerRadius * 2

        function clampedCornerRadius() {
            var maxCornerRadius = Math.min(rootItem.width, rootItem.height) / 2 + rootItem.glowRadius;
            return Math.max(0, Math.min(rootItem.cornerRadius, maxCornerRadius));
        }

        property color color: rootItem.color
        property real inverseSpread: 1.0 - rootItem.spread
        property real relativeSizeX: ((inverseSpread * inverseSpread) * rootItem.glowRadius + cornerRadius * 2.0) / width
        property real relativeSizeY: relativeSizeX * (width / height)
        property real spread: rootItem.spread / 2.0
        property real cornerRadius: clampedCornerRadius()

        fragmentShader: 'qrc:/Qcm/Material/assets/shader/rect_glow.frag.qsb'
    }
}

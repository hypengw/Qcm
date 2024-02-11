import QtQuick
import Qcm.Material as MD

// https://github.com/material-components/material-web/blob/v1.2.0/elevation/internal/_elevation.scss

Item {
    id: effect

    property var source

    property int elevation: 0

    property bool fullWidth: false
    property bool fullHeight: false

    readonly property Item sourceItem: source.sourceItem

    property var _shadows: _defaultShadows

    readonly property var _defaultShadows: [
        { // 0
            angularValues: [
                {offset: 0, blur: 0, spread: 0},
                {offset: 0, blur: 0, spread: 0},
                {offset: 0, blur: 0, spread: 0}
            ],
            strength: 0.05
        },
        { // 1
            angularValues: [
                {offset: 1, blur: 3, spread: 0},
                {offset: 1, blur: 1, spread: 0},
                {offset: 2, blur: 1, spread: -1}
            ],
            strength: 0.05
        },
        { // 2
            angularValues: [
                {offset: 1, blur: 5, spread: 0},
                {offset: 2, blur: 2, spread: 0},
                {offset: 3, blur: 1, spread: -2}
            ],
            strength: 0.05
        },
        { // 3
            angularValues: [
                {offset: 1, blur: 8, spread: 0},
                {offset: 3, blur: 4, spread: 0},
                {offset: 3, blur: 3, spread: -2}
            ],
            strength: 0.05
        },
        { // 4
            angularValues: [
                {offset: 2, blur: 4, spread: -1},
                {offset: 4, blur: 5, spread: 0},
                {offset: 1, blur: 10, spread: 0}
            ],
            strength: 0.05
        },
        { // 5
            angularValues: [
                {offset: 3, blur: 5, spread: -1},
                {offset: 5, blur: 8, spread: 0},
                {offset: 1, blur: 14, spread: 0}
            ],
            strength: 0.05
        },
        { // 6
            angularValues: [
                {offset: 3, blur: 5, spread: -1},
                {offset: 6, blur: 10, spread: 0},
                {offset: 1, blur: 18, spread: 0}
            ],
            strength: 0.05
        },
        { // 7
            angularValues: [
                {offset: 4, blur: 5, spread: -2},
                {offset: 7, blur: 10, spread: 1},
                {offset: 2, blur: 16, spread: 1}
            ],
            strength: 0.05
        },
        { // 8
            angularValues: [
                {offset: 5, blur: 5, spread: -3},
                {offset: 8, blur: 10, spread: 1},
                {offset: 3, blur: 14, spread: 2}
            ],
            strength: 0.05
        },
        { // 9
            angularValues: [
                {offset: 5, blur: 6, spread: -3},
                {offset: 9, blur: 12, spread: 1},
                {offset: 3, blur: 16, spread: 2}
            ],
            strength: 0.05
        },
        { // 10
            angularValues: [
                {offset: 6, blur: 6, spread: -3},
                {offset: 10, blur: 14, spread: 1},
                {offset: 4, blur: 18, spread: 3}
            ],
            strength: 0.05
        },
        { // 11
            angularValues: [
                {offset: 6, blur: 7, spread: -4},
                {offset: 11, blur: 15, spread: 1},
                {offset: 4, blur: 20, spread: 3}
            ],
            strength: 0.05
        },
        { // 12
            angularValues: [
                {offset: 7, blur: 8, spread: -4},
                {offset: 12, blur: 17, spread: 2},
                {offset: 5, blur: 22, spread: 4}
            ],
            strength: 0.05
        }
    ]

    readonly property var _shadow: _shadows[Math.max(0, Math.min(elevation, _shadows.length - 1))]

    Item {
        property int margin: -100

        x: margin
        y: margin
        width: parent.width - 2 * margin
        height: parent.height - 2 * margin

        // By rendering as a layer, the shadow will never show through the source item,
        // even when the source item's opacity is less than 1
        layer.enabled: true

        // The box shadows automatically pick up the size of the source Item and not
        // the size of the parent, so we don't need to worry about the extra padding
        // in the parent Item

        // key box
        MD.BoxShadow {
            offsetY: effect._shadow.angularValues[0].offset
            blurRadius: effect._shadow.angularValues[0].blur
            spreadRadius: effect._shadow.angularValues[0].spread
            strength: effect._shadow.strength
            color: Qt.rgba(0,0,0, 0.2)

            fullWidth: effect.fullWidth
            fullHeight: effect.fullHeight
            source: effect.sourceItem
        }
        
        // ambient box
        MD.BoxShadow {
            offsetY: effect._shadow.angularValues[1].offset
            blurRadius: effect._shadow.angularValues[1].blur
            spreadRadius: effect._shadow.angularValues[1].spread
            strength: effect._shadow.strength
            color: Qt.rgba(0,0,0, 0.14)

            fullWidth: effect.fullWidth
            fullHeight: effect.fullHeight
            source: effect.sourceItem
        }

        MD.BoxShadow {
            offsetY: effect._shadow.angularValues[2].offset
            blurRadius: effect._shadow.angularValues[2].blur
            spreadRadius: effect._shadow.angularValues[2].spread
            strength: effect._shadow.strength
            color: Qt.rgba(0,0,0, 0.12)

            fullWidth: effect.fullWidth
            fullHeight: effect.fullHeight
            source: effect.sourceItem
        }

        ShaderEffect {
            property alias source: effect.source

            x: (parent.width - width)/2
            y: (parent.height - height)/2
            width: effect.sourceItem.width
            height: effect.sourceItem.height
        }
    }
}
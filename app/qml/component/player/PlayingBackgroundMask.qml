import QtQuick

Rectangle {
    id: root

    // The xxhdpi nine-patch stretches row 483; the other 1361 rows retain their dp height.
    readonly property real borderScale: Math.min(1, height / (1361 / 3))

    function maskPosition(row: real): real {
        if (height <= 0)
            return 0;
        return row < 483 ? (row + 0.5) / 3 * borderScale / height : 1 - (1362 - row - 0.5) / 3 * borderScale / height;
    }

    // These samples bound the source-row alpha interpolation error to 1/255, including the endpoints.
    gradient: Gradient {
        GradientStop {
            position: root.maskPosition(0)
            color: "#7a000000"
        }
        GradientStop {
            position: root.maskPosition(140)
            color: "#7b000000"
        }
        GradientStop {
            position: root.maskPosition(287)
            color: "#74000000"
        }
        GradientStop {
            position: root.maskPosition(362)
            color: "#6f000000"
        }
        GradientStop {
            position: root.maskPosition(379)
            color: "#6f000000"
        }
        GradientStop {
            position: root.maskPosition(482)
            color: "#6b000000"
        }
        GradientStop {
            position: root.maskPosition(484)
            color: "#6b000000"
        }
        GradientStop {
            position: root.maskPosition(781)
            color: "#6b000000"
        }
        GradientStop {
            position: root.maskPosition(881)
            color: "#70000000"
        }
        GradientStop {
            position: root.maskPosition(890)
            color: "#70000000"
        }
        GradientStop {
            position: root.maskPosition(986)
            color: "#7a000000"
        }
        GradientStop {
            position: root.maskPosition(1073)
            color: "#87000000"
        }
        GradientStop {
            position: root.maskPosition(1115)
            color: "#8f000000"
        }
        GradientStop {
            position: root.maskPosition(1265)
            color: "#ad000000"
        }
        GradientStop {
            position: root.maskPosition(1291)
            color: "#b0000000"
        }
        GradientStop {
            position: root.maskPosition(1297)
            color: "#b2000000"
        }
        GradientStop {
            position: root.maskPosition(1360)
            color: "#bb000000"
        }
        GradientStop {
            position: root.maskPosition(1361)
            color: "#b8000000"
        }
    }
}

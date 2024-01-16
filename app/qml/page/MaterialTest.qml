import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Qcm.Material as MD
import QtQuick.Controls.Material.impl as MDImpl

MD.Page {
    padding: 16

    MD.Flickable {
        id: flick
        anchors.fill: parent
        clip: true
        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 8

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
                    MD.Divider {
                    }

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
                    width: 200
                    height: 200
                    radius: 100
                    smooth: false
                    color: MD.Token.color.primary
                }
                Rectangle {
                    width: 100
                    height: 100
                    radius: 50
                    color: MD.Token.color.primary
                }
            }
            RowLayout {
                spacing: 24
                Rectangle {
                    id: rect_clip
                    width: 200
                    height: 200
                    color: MD.Token.color.primary

                    layer.enabled: true
                    layer.effect: MD.RoundClip {
                        radius: [slider_c1.value, slider_c2.value, slider_c3.value, slider_c4.value]
                        size: rect_clip.height
                        smoothing: slider_smoothing.value
                    }
                }
                ColumnLayout {
                    spacing: 4
                    MSlider {
                        id: slider_c1
                        text: 'lt'
                        from: 0
                        to: rect_clip.height / 2
                    }
                    MSlider {
                        id: slider_c2
                        text: 'rt'
                        from: 0
                        to: rect_clip.height / 2
                    }
                    MSlider {
                        id: slider_c3
                        text: 'lb'
                        from: 0
                        to: rect_clip.height / 2
                    }
                    MSlider {
                        id: slider_c4
                        text: 'rb'
                        from: 0
                        to: rect_clip.height / 2
                    }
                    MSlider {
                        id: slider_smoothing
                        text: 'smoothing'
                        from: 0.1
                        to: 10
                        value: 1
                    }
                }
            }
            RowLayout {
                Layout.leftMargin: 32
                Layout.topMargin: 32
                Layout.bottomMargin: 32
                spacing: 32
                Rectangle {
                    id: rect_shadow
                    radius: slider_shadow_radius.value
                    width: 100
                    height: 100
                    color: MD.Token.color.primary
                }
                Item {
                    implicitWidth: 200
                    implicitHeight: implicitWidth

                    MDImpl.BoxShadow {
                        anchors.centerIn: parent
                        source: rect_shadow
                        offsetX: 0
                        offsetY: 0
                        blurRadius: slider_shadow_blur_radius.value

                        spreadRadius: slider_shadow_spread_radius.value
                        strength: slider_shadow_strength.value
                        color: rect_shadow.color
                        fullWidth: true
                        fullHeight: true
                    }
                }

                ColumnLayout {
                    MD.Slider {
                        id: slider_shadow_radius
                        from: 0
                        to: rect_shadow.height / 2
                        value: 0
                    }
                    MD.Slider {
                        id: slider_shadow_blur_radius
                        from: 0
                        to: rect_shadow.height / 2
                        value: 0
                    }
                    MD.Slider {
                        id: slider_shadow_spread_radius
                        from: 0
                        to: rect_shadow.height / 2
                        value: 0
                    }
                    MD.Slider {
                        id: slider_shadow_strength
                        from: 0
                        to: 1
                        value: 0.2
                    }
                }
            }
            RowLayout {
                Layout.leftMargin: 32
                Layout.topMargin: 32
                Layout.bottomMargin: 32
                spacing: 32
                Item {
                    implicitWidth: 200
                    implicitHeight: implicitWidth

                    MD.RectangularGlow {
                        anchors.centerIn: parent
                        width: 100
                        height: width
                        color: MD.Token.color.primary
                        glowRadius: slider_glow_glowradius.value
                        cornerRadius: slider_glow_cornerradius.value
                        spread: slider_glow_spread.value
                    }
                }

                ColumnLayout {
                    MD.Slider {
                        id: slider_glow_glowradius
                        from: 0
                        to: 50
                        value: 0
                    }
                    MD.Slider {
                        id: slider_glow_cornerradius
                        from: 0
                        to: 50
                        value: 0
                    }
                    MD.Slider {
                        id: slider_glow_spread
                        from: 0
                        to: 1.0
                        value: 0
                    }
                }
            }
        }
    }

    component MSlider: RowLayout {
        property string text
        property alias to: slider.to
        property alias from: slider.from
        property alias value: slider.value
        MD.Text {
            text: parent.text
        }
        MD.Slider {
            id: slider
        }
    }
}

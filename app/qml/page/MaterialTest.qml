import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD
import org.kde.kirigami as Kirigami

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
                    }
                }
                ColumnLayout {
                    spacing: 4
                    MD.Slider {
                        id: slider_c1
                        from: 0
                        to: rect_clip.height / 2
                    }
                    MD.Slider {
                        id: slider_c2
                        from: 0
                        to: rect_clip.height / 2
                    }
                    MD.Slider {
                        id: slider_c3
                        from: 0
                        to: rect_clip.height / 2
                    }
                    MD.Slider {
                        id: slider_c4
                        from: 0
                        to: rect_clip.height / 2
                    }
                }
            }
        }
    }
}

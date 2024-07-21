import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.ListItem {
    id: root

    readonly property bool is_playing: QA.Global.playlist.cur.itemId === model_.song.itemId
    property QA.t_program model_: modelData
    property string subtitle: ''

    // enabled: model_.canPlay
    highlighted: is_playing
    heightMode: MD.Enum.ListItemTwoLine

    contentItem: RowLayout {
        spacing: 16

        StackLayout {
            Layout.fillHeight: false
            Layout.fillWidth: false
            Layout.minimumWidth: item_font_metrics.advanceWidth(root.count.toString()) + 2
            currentIndex: 0

            Binding on currentIndex  {
                value: 1
                when: root.is_playing
            }

            MD.FontMetrics {
                id: item_font_metrics
                typescale: MD.Token.typescale.body_medium
            }

            MD.Text {
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                typescale: MD.Token.typescale.body_medium
                opacity: 0.6
                text: root.model_.serialNum
            }
            MD.Icon {
                name: MD.Token.icon.equalizer
                size: 24
                MD.MatProp.textColor: MD.Token.color.primary
                horizontalAlignment: Qt.AlignHCenter
            }
        }
        ColumnLayout {
            spacing: 0
            MD.Text {
                Layout.fillWidth: true
                text: root.model_.name
                typescale: MD.Token.typescale.body_large
                verticalAlignment: Qt.AlignVCenter
            }
            RowLayout {
                Repeater {
                    model: root.model_.tags

                    delegate: ColumnLayout {
                        QA.SongTag {
                            tag: modelData
                        }
                    }
                }
                MD.Text {
                    id: subtitle_label
                    Layout.fillWidth: true
                    verticalAlignment: Qt.AlignVCenter
                    typescale: MD.Token.typescale.body_medium
                    color: MD.MatProp.supportTextColor
                    text: root.subtitle ? root.subtitle : `${Qt.formatDateTime(root.model_.createTime, 'yyyy.MM.dd')} - ${QA.Global.join_name(root.model_.song.artists, '/')}`
                }
            }
        }
        MD.Text {
            typescale: MD.Token.typescale.body_medium
            text: Qt.formatDateTime(root.model_.duration, 'mm:ss')
            verticalAlignment: Qt.AlignVCenter
        }
        RowLayout {
            spacing: 0

            MD.IconButton {
                icon.name: MD.Token.icon.more_vert

                onClicked: {
                    QA.Global.show_popup('qrc:/Qcm/App/qml/menu/ProgramMenu.qml', {
                            "program": model_,
                            "y": height
                        }, this);
                }
            }
        }
    }
}

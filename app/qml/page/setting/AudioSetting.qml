pragma ComponentBehavior: Bound
import QtCore
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

QA.SettingBasePage {
    id: root
    title: qsTr('audio')

    flickable.contentHeight: m_audio.height

    Settings {
        id: settings_audio
        category: 'audio'
        property int streaming_quality: QA.Enum.AQExhigh
        Component.onCompleted: {}
    }

    QA.SettingSection {
        id: m_audio
        width: parent.width
        height: implicitHeight
        spacing: 2

        QA.SettingRow {
            text: `${qsTr('Fade When Play/Pause')}: ${QA.Global.player.fadeTime} ms`
            font.capitalization: Font.MixedCase
            canInput: false
            below: MD.SliderM2 {
                id: m_slider_fade
                Layout.fillWidth: true
                from: 0
                stepSize: 20
                to: 1000
                value: QA.Global.player.fadeTime
                onValueChangeRequested: requestedValue => QA.Global.player.fadeTime = requestedValue
            }
        }
        QA.SettingRow {
            id: qualityRow
            text: qsTr('streaming quality')

            trailing: MD.ComboBox {
                id: m_comb_playing_quality

                Binding {
                    qualityRow.canInput: !m_comb_playing_quality.popup.visible
                }

                Connections {
                    target: qualityRow
                    function onClicked() {
                        m_comb_playing_quality.popup.open();
                    }
                }

                textRole: "text"
                valueRole: "value"
                currentValue: settings_audio.streaming_quality
                onSelectionChangeRequested: (index, value) => settings_audio.streaming_quality = value

                model: ListModel {}

                Component.onCompleted: {
                    [
                        {
                            "text": qsTr('Standard'),
                            "value": QA.Enum.AQStandard
                        },
                        {
                            "text": qsTr('Higher'),
                            "value": QA.Enum.AQHigher
                        },
                        {
                            "text": qsTr('Exhigh'),
                            "value": QA.Enum.AQExhigh
                        },
                        {
                            "text": qsTr('Lossless'),
                            "value": QA.Enum.AQLossless
                        }
                    ].map(el => model.append(el));
                }

            }
        }
    }
}

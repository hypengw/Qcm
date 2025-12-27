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
            text: `${qsTr('Fade When Play/Pause')}: ${m_slider_fade.value} ms`
            font.capitalization: Font.MixedCase
            canInput: false
            below: MD.Slider {
                id: m_slider_fade
                Layout.fillWidth: true
                from: 0
                stepSize: 20
                to: 1000
                onMoved: {
                    QA.Global.player.fadeTime = value;
                }
                Component.onCompleted: {
                    value = QA.Global.player.fadeTime;
                }
            }
        }
        QA.SettingRow {
            text: qsTr('streaming quality')
            canInput: !m_comb_playing_quality.popup.visible

            trailing: MD.ComboBox {
                id: m_comb_playing_quality
                signal clicked

                textRole: "text"
                valueRole: "value"

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
                    currentIndex = indexOfValue(settings_audio.streaming_quality);
                    settings_audio.streaming_quality = Qt.binding(() => {
                        return m_comb_playing_quality.currentValue;
                    });
                }

                onClicked: {
                    popup.open();
                }
            }
        }
    }
}

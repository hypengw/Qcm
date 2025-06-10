pragma ComponentBehavior: Bound
import QtCore
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    font.capitalization: Font.Capitalize
    title: qsTr('audio')
    bottomPadding: radius
    scrolling: !m_flick.atYBeginning

    MD.VerticalFlickable {
        id: m_flick
        anchors.fill: parent
        leftMargin: 0
        rightMargin: 0

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
            horizontalPadding: 16

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
}

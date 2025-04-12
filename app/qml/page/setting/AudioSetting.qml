import QtCore
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    font.capitalization: Font.Capitalize
    title: qsTr('settings')
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

        ColumnLayout {
            height: implicitHeight
            spacing: 12
            width: parent.width
            SettingSection {
                id: sec_audio
                Layout.fillWidth: true
                title: qsTr('audio')

                SettingRow {
                    Layout.fillWidth: true
                    text: `${qsTr('Fade When Play/Pause')}: ${slider_fade.value} ms`
                    font.capitalization: Font.MixedCase
                    canInput: false
                    belowItem: MD.Slider {
                        id: slider_fade
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
                SettingRow {
                    Layout.fillWidth: true
                    text: qsTr('streaming quality')
                    canInput: !comb_playing_quality.popup.visible

                    actionItem: MD.ComboBox {
                        id: comb_playing_quality
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
                                return comb_playing_quality.currentValue;
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
}

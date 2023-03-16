import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."

ItemDelegate {
    id: root

    required property int index
    required property int count
    required property var modelData
    readonly property int count_len: this.count.toString().length
    property string subtitle: ''

    contentItem: RowLayout {
        spacing: 16

        StackLayout {
            Layout.fillWidth: false
            Layout.fillHeight: false
            Layout.minimumWidth: Theme.font.w_unit * root.count_len + 2
            currentIndex: 0

            Label {
                horizontalAlignment: Qt.AlignRight
                text: index + 1
                opacity: 0.6
            }

            Label {
                horizontalAlignment: Qt.AlignRight
                font.family: Theme.font.icon_round.family
                font.pointSize: 16
                color: Theme.color.tertiary
                text: Theme.ic.equalizer
            }

            Binding on currentIndex {
                when: QA.playlist.cur.itemId === modelData.itemId
                value: 1
            }

        }

        ColumnLayout {
            Label {
                Layout.fillWidth: true
                text: root.modelData.name
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.pointSize: Theme.font.small(Theme.font.label_font)
                opacity: 0.6
                text: root.subtitle ? root.subtitle : QA.join_name(root.modelData.artists, '/')
            }

        }

        Label {
            text: Qt.formatDateTime(root.modelData.duration, 'mm:ss')
        }

        RoundButton {
            id: btn_menu

            //padding: 14
            //background.implicitWidth: 0
            //background.implicitHeight: 0
            text: Theme.ic.more_vert
            font.family: Theme.font.icon_round.family
            font.pointSize: 14
            flat: true
            onClicked: {
                menu.open();
            }

            Menu {
                id: menu

                y: btn_menu.height
                modal: true
                dim: false

                Action {
                    text: qsTr('Play next')
                    onTriggered: {
                        QA.playlist.appendNext(modelData);
                    }
                }

                Action {
                    text: qsTr('Show album')
                    onTriggered: {
                        QA.route(modelData.album.itemId);
                    }
                }

                Action {
                    text: qsTr('Show artist')
                    onTriggered: {
                        const artists = modelData.artists;
                        if (artists.length === 1)
                            QA.route(artists[0].itemId);
                        else
                            QA.show_popup('qrc:/QcmApp/qml/part/ArtistsPopup.qml', {
                            "model": artists
                        });
                    }
                }

            }

        }

    }

}

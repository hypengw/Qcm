import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QcmApp
import ".."

ItemDelegate {
    id: root

    required property int index
    required property int count
    required property var modelData
    readonly property int count_len: this.count.toString().length

    contentItem: RowLayout {
        spacing: 16

        Label {
            Layout.minimumWidth: Theme.font.w_unit * root.count_len + 2
            horizontalAlignment: Qt.AlignRight
            text: index + 1
            opacity: 0.6
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
                text: QA.join_name(root.modelData.artists, '/')
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
                        btn_menu.Window.window.route(modelData.album.itemId);
                    }
                }

                Action {
                    text: qsTr('Show artist')
                    onTriggered: {
                        const artists = modelData.artists;
                        if (artists.length === 1)
                            btn_menu.Window.window.route(artists[0].itemId);
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

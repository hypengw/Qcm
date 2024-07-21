import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Page {
    id: root
    title: `Create a Playlist`
    horizontalPadding: 24

    ColumnLayout {
        anchors.fill: parent
        spacing: 24

        ColumnLayout {
            MD.TextField {
                id: item_input
                Layout.fillWidth: true
            }
        }
        RegularExpressionValidator {
            id: item_valid
            regularExpression: /.+/
        }

        QNcm.PlaylistCreateQuerier {
            id: qr_playlist_create
            name: item_input.text
            autoReload: false
            onStatusChanged: {
                if (status === QA.enums.Finished) {
                    QA.App.playlistCreated();
                    MD.Util.closePopup(root);
                }
            }
        }

        RowLayout {
            Item {
                Layout.fillWidth: true
            }

            MD.Button {
                type: MD.Enum.BtText
                text: qsTr('cancel')
                onClicked: {
                    MD.Util.closePopup(root);
                }
            }

            MD.Button {
                type: MD.Enum.BtText
                text: qsTr('complete')
                onClicked: {
                    item_input.validator = item_valid;
                    if (item_input.acceptableInput) {
                        qr_playlist_create.query();
                    } else {
                        item_input.focus = true;
                    }
                }
            }
        }
    }
}

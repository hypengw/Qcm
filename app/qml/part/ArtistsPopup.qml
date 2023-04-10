import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "../component"
import ".."

MPopup {
    id: root

    required property var model

    title: qsTr('artists')
    width: 300

    MListView {
        Layout.fillHeight: true
        Layout.fillWidth: true
        clip: true
        currentIndex: -1
        implicitHeight: contentHeight
        model: root.model

        delegate: MItemDelegate {
            horizontalPadding: 8
            padding: 8
            width: ListView.view.width

            contentItem: RowLayout {
                spacing: 12

                RoundImage {
                    image: Image {
                        source: `image://ncm/${modelData.picUrl}`
                        sourceSize.height: 48
                        sourceSize.width: 48
                    }
                }
                Label {
                    Layout.fillWidth: true
                    text: modelData.name
                }
            }

            onClicked: {
                root.close();
                QA.route(modelData.itemId);
            }
        }
    }
}

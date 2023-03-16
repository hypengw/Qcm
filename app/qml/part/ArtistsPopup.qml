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
    height: Math.min(implicitHeight, parent.height * 0.8)
    width: 300

    ListView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        implicitHeight: contentHeight
        currentIndex: -1
        clip: true
        model: root.model

        delegate: MItemDelegate {
            width: ListView.view.width
            onClicked: {
                root.close();
                QA.route(modelData.itemId);
            }

            RowLayout {
                anchors.fill: parent

                RoundImage {

                    image: Image {
                        sourceSize.width: 48
                        sourceSize.height: 48
                        source: `image://ncm/${modelData.picUrl}`
                    }

                }

                Label {
                    text: modelData.name
                }

            }

        }

    }

}

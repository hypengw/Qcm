pragma ComponentBehavior: Bound
import QtCore
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root

    required property QA.item_id itemId
    readonly property var model: QA.App.providerStatus.itemById(itemId)
    readonly property var meta: QA.App.providerStatus.metaById(itemId)
    readonly property bool changed: model.name != m_tf_name.text || (m_auth_loader.item?.modified ?? false)

    font.capitalization: Font.Capitalize
    title: qsTr('provider edit')
    bottomPadding: radius
    scrolling: !m_flick.atYBeginning

    MD.VerticalFlickable {
        id: m_flick
        anchors.fill: parent
        bottomMargin: 24
        contentHeight: m_main.implicitHeight

        ColumnLayout {
            id: m_main
            anchors.centerIn: parent
            height: implicitHeight
            spacing: 12

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                MD.IconSvg {
                    Layout.alignment: Qt.AlignHCenter
                    sourceData: root.meta.svg
                    size: 42
                }
                MD.Label {
                    typescale: MD.Token.typescale.title_large
                    text: root.model.typeName
                }
            }

            MD.TextField {
                id: m_tf_name
                type: MD.Enum.TextFieldFilled
                Layout.minimumWidth: 300
                Layout.fillWidth: true
                placeholderText: qsTr('Provider Name')
                text: root.model.name
            }

            MD.HorizontalListView {
                id: m_auth_type_view
                Layout.fillWidth: true
                implicitHeight: 36
                visible: count > 1
                spacing: 8
                property list<var> models: [
                    {
                        name: qsTr('Username'),
                        comp: m_comp_username
                    },
                    {
                        name: qsTr('Phone'),
                        comp: m_comp_username
                    },
                    {
                        name: qsTr('Email'),
                        comp: m_comp_email
                    },
                    {
                        name: qsTr('Qr'),
                        comp: m_comp_qr
                    }
                ]
                currentIndex: 0
                model: root.meta.authTypes.map(el => models[el])
                delegate: MD.InputChip {
                    required property int index
                    required property var model
                    action: MD.Action {
                        icon.name: ''
                        checked: m_auth_type_view.currentIndex == index
                        onTriggered: {
                            m_auth_type_view.currentIndex = index;
                        }
                        text: model.name
                    }
                }
            }

            MD.TextField {
                id: m_tf_server
                Layout.fillWidth: true
                visible: root.meta.hasServerUrl
                type: MD.Enum.TextFieldFilled
                placeholderText: qsTr('Server Url')
            }

            Loader {
                id: m_auth_loader
                Layout.fillWidth: true
                sourceComponent: m_auth_type_view.model[m_auth_type_view.currentIndex].comp
            }

            RowLayout {
                MD.Button {
                    text: qsTr("reset")
                    onClicked: {
                        const it = m_auth_loader.item;
                        if (it?.reset)
                            it.reset();
                        m_tf_name.text = root.model.name;
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                MD.Button {
                    enabled: root.changed
                    text: qsTr("apply")
                }
            }
            Component {
                id: m_comp_email
                QA.AuthEmail {
                    anchors.fill: parent
                }
            }

            Component {
                id: m_comp_username
                QA.AuthUsername {
                    anchors.fill: parent
                }
            }
            Component {
                id: m_comp_qr
                QA.AuthQr {
                    query: m_add_query
                    name: m_tf_name.text
                    serverUrl: m_tf_server.text
                    typeName: root.meta.typeName
                    anchors.fill: parent
                }
            }
        }
    }
}

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD
import Qcm.App as QA

import Qcm.Msg as QM

MD.Page {
    id: root
    font.capitalization: Font.Capitalize
    title: qsTr('provider')
    bottomPadding: radius
    scrolling: !m_view.atYBeginning
    readonly property var libStatus: QA.App.providerStatus.libraryStatus

    MD.VerticalListView {
        id: m_view
        width: root.width
        leftMargin: 12
        rightMargin: 12
        topMargin: 8
        bottomMargin: 8
        spacing: 8
        expand: true

        model: QA.App.providerStatus

        delegate: MD.Control {
            id: m_item
            required property int index
            required property var model
            readonly property var libraries: model.librariesData
            readonly property bool syncing: model.syncStatus.state == QM.SyncState.SYNC_STATE_SYNCING

            width: ListView.view.contentWidth
            topPadding: 0
            bottomPadding: 12

            QA.SyncQuery {
                id: m_query
            }

            contentItem: Column {
                spacing: 0

                MD.Control {
                    height: syncing ? implicitHeight : 0
                    clip: true
                    width: parent.width
                    verticalPadding: 4
                    horizontalPadding: MD.Token.shape.corner.medium
                    Behavior on height {
                        NumberAnimation {
                            duration: 300
                        }
                    }

                    contentItem: MD.Label {
                        text: {
                            const s = model.syncStatus;
                            return `album: ${s.album} artist: ${s.artist} song: ${s.song}`;
                        }
                        color: MD.MProp.color.on_primary
                        typescale: MD.Token.typescale.label_medium
                    }

                    background: MD.Rectangle {
                        color: MD.Token.color.primary
                        corners: MD.Util.corner(MD.Token.shape.corner.medium, MD.Token.shape.corner.medium, 0, 0)
                    }
                }

                MD.Space {
                    spacing: syncing ? 8 : 12
                }

                Column {
                    anchors {
                        left: parent.left
                        right: parent.right
                        leftMargin: 12
                        rightMargin: 12
                    }

                    spacing: 4

                    RowLayout {
                        width: parent.width
                        MD.IconSvg {
                            sourceData: QA.App.providerStatus.svg(index)
                            size: 24
                        }
                        MD.Text {
                            text: model.name
                            typescale: MD.Token.typescale.title_medium
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        MD.BusyButton {
                            type: MD.Enum.BtOutlined
                            text: qsTr('sync')
                            busy: m_query.querying || syncing
                            background.implicitHeight: 32
                            onClicked: {
                                m_query.providerId = model.itemId;
                                m_query.reload();
                            }
                        }
                    }
                    MD.IconLabel {
                        clip: true
                        visible: text
                        anchors.leftMargin: 12
                        icon_name: MD.Token.icon.error
                        icon_color: MD.Token.color.error
                        text: {
                            const state = model.syncStatus.state;
                            switch (state) {
                            case QM.SyncState.SYNC_STATE_NOT_AUTH:
                                return 'not auth';
                            case QM.SyncState.SYNC_STATE_NETWORK_ERROR:
                                return 'network error';
                            case QM.SyncState.SYNC_STATE_UNKNOWN_ERROR:
                                return 'unknown error';
                            }
                            return '';
                        }
                        color: MD.Token.color.error
                    }
                    Flow {
                        width: parent.width
                        spacing: 8
                        Repeater {
                            model: m_item.libraries
                            MD.FilterChip {
                                required property var modelData
                                text: modelData.name
                                checkable: false
                                checked: root.libStatus.actived(modelData.libraryId)
                                onClicked: {
                                    root.libStatus.setActived(modelData.libraryId, !checked);
                                }
                            }
                        }
                    }
                }
            }
            background: MD.Rectangle {
                color: MD.Token.color.surface_container
                radius: MD.Token.shape.corner.medium
            }
        }
    }
}

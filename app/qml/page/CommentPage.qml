pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    required property QA.item_id itemId
    required property QA.CommentStore store
    property string parentId: ""
    readonly property QA.EntityView entity: QA.Store.item(itemId)
    readonly property bool replies: parentId.length > 0
    implicitWidth: 600
    implicitHeight: 640
    padding: 0
    title: replies ? qsTr("Replies") : qsTr("Comments")
    scrolling: !m_view.atYBeginning
    actions: [MD.Action {
        text: qsTr("Refresh")
        icon.name: MD.Token.icon.refresh
        busy: m_query.querying
        enabled: !m_query.querying
        onTriggered: m_query.reload()
    }]

    function openReplies(id) {
        root.MD.MProp.page.pushItem('Qcm.App/CommentPage', {
            itemId: root.itemId, store: root.store, parentId: id
        });
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        MD.Text {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: 8
            text: root.entity?.name ?? ""
            visible: text.length > 0
            textFormat: Text.PlainText
            maximumLineCount: 2
            typescale: MD.Token.typescale.title_medium
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            visible: m_query.supported
            spacing: 8
            MD.FilterChip {
                visible: !root.replies && m_query.orders.includes(0)
                text: qsTr("Latest")
                selected: m_query.order === 0
                onSelectionRequested: m_query.order = 0
            }
            MD.FilterChip {
                visible: !root.replies && m_query.orders.includes(1)
                text: qsTr("Hot")
                selected: m_query.order === 1
                onSelectionRequested: m_query.order = 1
            }
            Item { Layout.fillWidth: true }
            MD.Text {
                visible: m_query.total !== undefined && m_query.total !== null
                text: qsTr("%1 comments").arg(m_query.total ?? "")
                typescale: MD.Token.typescale.label_medium
            }
        }
        MD.PageVerticalLazyList {
            id: m_view
            objectName: "commentsList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            source: QA.ItemModelSource {
                model: m_query.model
            }
            pageMarginEdges: Qt.LeftEdge | Qt.RightEdge | Qt.BottomEdge
            header: Loader {
                width: m_view.contentWidth
                active: root.replies
                sourceComponent: QA.CommentDelegate {
                    width: m_view.contentWidth
                    commentView: root.store?.view(root.parentId) ?? null
                    store: root.store
                    canReply: false
                }
            }
            delegate: QA.CommentDelegate {
                required property MD.LazyRow row
                commentView: row.value.commentView
                width: m_view.contentWidth
                store: root.store
                canReply: m_query.canReply
                onRepliesRequested: id => root.openReplies(id)
            }
            onContentYChanged: {
                if (moving && atYEnd && !m_query.error.length)
                    m_query.fetchMore();
            }
            footer: ColumnLayout {
                width: m_view.contentWidth
                spacing: 8
                MD.Text {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    visible: root.replies && m_query.loaded && !m_query.querying && !m_query.error.length && m_query.model.count === 0
                    text: m_query.supported ? qsTr("No replies yet") : qsTr("Replies are not supported for this item")
                    typescale: MD.Token.typescale.body_medium
                }
                MD.CircularIndicator {
                    Layout.alignment: Qt.AlignHCenter
                    visible: root.replies && m_query.querying
                    running: visible
                    indeterminate: true
                }
                MD.Text {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    visible: m_query.error.length > 0
                    text: m_query.error
                    textFormat: Text.PlainText
                    typescale: MD.Token.typescale.body_medium
                    color: MD.Token.color.error
                }
                MD.BusyButton {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 16
                    visible: m_query.hasMore || m_query.error.length > 0
                    busy: m_query.querying
                    enabled: !m_query.querying
                    text: m_query.error.length ? qsTr("Retry") : qsTr("Load more")
                    onClicked: m_query.hasMore ? m_query.fetchMore() : m_query.reload()
                }
            }
            ColumnLayout {
                anchors.centerIn: parent
                width: Math.max(0, parent.width - 48)
                visible: m_query.model.count === 0 && !root.replies
                MD.CircularIndicator {
                    Layout.alignment: Qt.AlignHCenter
                    visible: m_query.querying
                    running: visible
                    indeterminate: true
                }
                MD.Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    visible: m_query.loaded && !m_query.error.length && !m_query.querying
                    text: m_query.supported ? qsTr("No comments yet") : qsTr("Comments are not supported for this item")
                    typescale: MD.Token.typescale.body_large
                }
            }
        }
    }
    QA.CommentsQuery {
        id: m_query
        itemId: root.itemId
        store: root.store
        parentId: root.parentId
        Component.onCompleted: delayReload()
    }
    Component.onDestruction: m_query.stop()
}

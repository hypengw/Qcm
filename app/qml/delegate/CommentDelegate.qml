pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD
import Qcm.App as QA

MD.Control {
    id: root
    required property QA.CommentView commentView
    required property QA.CommentStore store
    property bool canReply: false
    readonly property var entry: commentView?.value
    signal repliesRequested(string id)
    horizontalPadding: 16
    verticalPadding: 12
    contentItem: RowLayout {
        spacing: 12
        Item {
            Layout.preferredWidth: 36
            Layout.preferredHeight: 36
            Layout.alignment: Qt.AlignTop
            MD.Icon {
                anchors.centerIn: parent
                name: MD.Token.icon.person
                size: 24
            }
            QA.Image {
                anchors.fill: parent
                source: root.entry?.author?.avatarUrl ?? ""
                displaySize: Qt.size(36, 36)
                radius: 18
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            MD.Text {
                Layout.fillWidth: true
                text: root.entry?.author?.name || qsTr("Unknown user")
                textFormat: Text.PlainText
                maximumLineCount: 2
                typescale: MD.Token.typescale.label_large
            }
            MD.Text {
                Layout.fillWidth: true
                visible: root.entry?.hasTimeMs ?? false
                text: root.commentView ? Qt.formatDateTime(root.commentView.time, "yyyy.MM.dd hh:mm") : ""
                typescale: MD.Token.typescale.label_small
                color: MD.Token.color.on_surface_variant
            }
            MD.Text {
                Layout.fillWidth: true
                text: root.entry?.deleted ? qsTr("Comment deleted") : root.entry?.content ?? ""
                textFormat: Text.PlainText
                wrapMode: Text.WrapAnywhere
                maximumLineCount: -1
                typescale: MD.Token.typescale.body_large
            }
            MD.Text {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                visible: root.entry?.hasQuote ?? false
                text: {
                    const quote = root.entry?.quote;
                    if (!quote) return "";
                    return `${quote.author.name || qsTr("Unknown user")}: ${quote.deleted ? qsTr("Comment deleted") : quote.content}`;
                }
                textFormat: Text.PlainText
                maximumLineCount: 4
                typescale: MD.Token.typescale.body_medium
                color: MD.Token.color.on_surface_variant
            }
            Repeater {
                model: root.entry?.previews ?? []
                MD.Text {
                    required property var modelData
                    readonly property QA.CommentView preview: root.store?.view(modelData.id_proto) ?? null
                    Layout.fillWidth: true
                    text: `${preview?.value.author.name || qsTr("Unknown user")}: ${preview?.value.content ?? ""}`
                    textFormat: Text.PlainText
                    maximumLineCount: 2
                    typescale: MD.Token.typescale.body_medium
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                MD.BusyIconButton {
                    objectName: "commentLike"
                    text: root.entry?.liked ? qsTr("Unlike") : qsTr("Like")
                    icon.name: MD.Token.icon.thumb_up
                    icon.fill: root.entry?.liked ?? false
                    busy: root.commentView?.pending ? MD.Enum.Busy : MD.Enum.Idle
                    enabled: (root.store?.canLike ?? false) && (root.entry?.hasLiked ?? false) && !root.entry?.deleted
                             && !root.commentView?.pending && !root.commentView?.uncertain
                    hoverEnabled: true
                    MD.ToolTip.text: text
                    MD.ToolTip.visible: mdState.interactions.hovered && !mdState.interactions.pressed
                    onClicked: root.store?.setLiked(root.entry.id_proto, !root.entry.liked)
                }
                MD.Text {
                    visible: root.entry?.hasLikeCount ?? false
                    text: (root.entry?.likeCount ?? 0).toString()
                    typescale: MD.Token.typescale.label_medium
                }
                Item { Layout.fillWidth: true }
                MD.Button {
                    visible: root.canReply && (root.entry?.replyCount ?? 0) > 0
                    text: qsTr("%1 replies").arg(root.entry?.replyCount ?? 0)
                    onClicked: root.repliesRequested(root.entry.id_proto)
                }
            }
            MD.Text {
                Layout.fillWidth: true
                visible: (root.commentView?.error.length ?? 0) > 0
                text: root.commentView?.error ?? ""
                textFormat: Text.PlainText
                typescale: MD.Token.typescale.body_small
                color: MD.Token.color.error
            }
        }
    }
}

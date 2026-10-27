import QtQuick
import Qcm.App as QA

QA.PagePopup {
    id: root
    required property QA.item_id itemId
    implicitWidth: 600
    backGestureEnabled: true
    source: 'Qcm.App/CommentPage'
    props: ({itemId: root.itemId, store: m_store})
    QA.CommentStore { id: m_store }
    Connections {
        target: m_store
        function onReactionFailed(message) { QA.Action.toast(message, 3000); }
    }
    onAboutToHide: {
        contentItem.abortPreview();
        m_store.close();
    }
}

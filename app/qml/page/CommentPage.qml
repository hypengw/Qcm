import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0
    title: m_view.model.total ? `Comments(${m_view.model.total})` : 'Comment'
    bottomPadding: radius
    scrolling: !m_view.atYBeginning

    property QA.t_id itemId

    MD.ListView {
        id: m_view
        anchors.fill: parent
        expand: true
        busy: m_query.status === QA.enums.Querying
        model: m_query.data

        delegate: QA.CommentDelegate {}

        QA.CommentsQuery {
            id: m_query
            delay: false
            itemId: root.itemId
        }
    }
}

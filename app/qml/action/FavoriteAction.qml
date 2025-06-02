import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    property alias itemId: m_query.itemId
    enabled: itemId.valid
    checked: {
        const ex = QA.Store.extra(itemId);
        return ex?.dynamic?.is_favorite ?? false;
    }
    icon.name: checked ? MD.Token.icon.favorite : MD.Token.icon.favorite_border
    busy: m_query.querying
    onTriggered: {
        if (!busy)
            m_query.reload();
    }
    QA.SetFavoriteQuery {
        id: m_query
        favorite: !root.checked
    }
}

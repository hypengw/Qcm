import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    icon.name: MD.Token.icon.filter_list
    text: qsTr('filter')
    enabled: model

    property QA.FilterRuleModel model

    onTriggered: {
        const msg = QA.Util.routeMsg();
        msg.props = {
            model: root.model
        };
        if (model instanceof QA.AlbumFilterRuleModel) {
            msg.dst = 'Qcm.App/AlbumFilterDialog';
        }
        QA.Action.openPopup(msg);
    }
}

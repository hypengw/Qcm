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
        msg.dst = 'Qcm.App/FilterDialog';
        if (model instanceof QA.AlbumFilterRuleModel || model instanceof QA.ArtistFilterRuleModel || model instanceof QA.MixFilterRuleModel)
        // valid
        {} else {
            return;
        }
        QA.Action.openPopup(msg);
    }
}

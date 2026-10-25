import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

QA.RecommendationCard {
    id: root
    required property var model
    readonly property var radio: model.radio
    required property QA.HomeRecommendationsQuery query
    objectName: "radioCard/" + model.key
    itemId: model.itemId
    text: model.name
    detail: model.description
    radioCover: true
    dynamicQueue: radio ? query.radioQueue(radio) : null
    playAction: MD.Action {
        icon.name: MD.Token.icon.play_arrow
        checked: true
        onTriggered: {
            if (root.radio)
                root.query.activateRadio(root.radio);
        }
    }
    onClicked: {
        if (radio)
            query.activateRadio(radio);
    }
}

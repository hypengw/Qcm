import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

QA.RecommendationCard {
    id: root
    required property var model
    required property var radio
    required property QA.HomeRecommendationsQuery query
    objectName: "radioCard/" + model.itemId.id
    itemId: model.itemId
    text: model.name
    detail: model.description
    radioCover: true
    dynamicQueue: query.radioQueue(radio)
    playAction: MD.Action {
        icon.name: MD.Token.icon.play_arrow
        checked: true
        onTriggered: root.query.activateRadio(root.radio)
    }
    onClicked: query.activateRadio(radio)
}

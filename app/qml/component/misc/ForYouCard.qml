import QtQuick
import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

QA.RecommendationCard {
    id: root
    required property var model
    required property QA.HomeRecommendationsQuery query
    objectName: "forYouCard/" + model.key
    itemId: model.itemId
    text: model.title
    detail: model.detail
    badge: model.badge
    coverId: model.coverId
    radioCover: model.coverKind === QM.CardCoverKind.CARD_COVER_KIND_RADIO_CURRENT
    dynamicQueue: model.itemId.type === QA.Enum.ItemRadioQueue ? query.radioQueue(model.radio) : null
    smallPlayButton: model.style === QM.CardStyle.CARD_STYLE_MEDIA
    playButtonAlignment: smallPlayButton ? Qt.AlignRight | Qt.AlignBottom : Qt.AlignCenter
    playAction: model.playAction === QM.CardAction.CARD_ACTION_NONE ? null : m_play_action
    onClicked: query.activate(model.card)

    MD.Action {
        id: m_play_action
        icon.name: MD.Token.icon.play_arrow
        checked: true
        onTriggered: root.query.activate(root.model.card, true)
    }
}

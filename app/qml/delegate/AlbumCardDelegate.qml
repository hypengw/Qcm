pragma ComponentBehavior: Bound
import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

QA.ImagePlayCard {
    id: root
    required property MD.LazyRow row
    itemRow: row
    readonly property var model: row.value
    itemId: model.itemId
    smallPlayButton: true
    playButtonAlignment: Qt.AlignRight | Qt.AlignBottom
    image.source: QA.Util.image_url(model.itemId)
    text: model.name
    subText: QA.Util.joinName(model.artists, "/") || qsTr("%1 tracks").arg(model.trackCount)
    picWidth: width
}

import QtQuick

import Qcm.Material as MD


MD.ListItem {
    id: root
    width: ListView.view.width
    text: model.user.name
    supportText: Qt.formatDateTime(model.time, 'yyyy.M.d')
    leader: MD.Image {
        radius: 8
        source: QA.Util.image_url(model.user.picUrl)
        sourceSize.height: 48
        sourceSize.width: 48
    }

    below: MD.Text {
        text: model.content
        maximumLineCount: -1
        typescale: MD.Token.typescale.body_large
    }
}

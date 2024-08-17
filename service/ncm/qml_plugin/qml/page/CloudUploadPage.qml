import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Qcm.Service.Ncm as QNCM
import Qcm.Material as MD

MD.Page {
    id: root
    font.capitalization: Font.Capitalize
    title: qsTr('upload queue')

    elevation: MD.Token.elevation.level0

    MD.ListView {
        model: m_api.data
    }

    QNCM.CloudUploadApi {
        id: m_api
        Component.onCompleted: {
            upload(encodeURI('file:///var/home/out/Music/local music/Alone/01 Alone.m4a'));
        }
    }
}
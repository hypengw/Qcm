import QtQuick
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Page {
    id: root
    font.capitalization: Font.Capitalize
    title: qsTr('settings')

    elevation: MD.Token.elevation.level0

    MD.ListView {
        model: m_api.data
    }

    QNcm.CloudUploadApi {
        id: m_api
    }
}
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Qcm.Service.Ncm as QNCM
import Qcm.Material as MD

MD.Page {
    id: root
    font.capitalization: Font.Capitalize
    title: qsTr('upload queue')

    elevation: MD.Token.elevation.level0

    MD.ListView {
 //       model: m_api.data
    }


}
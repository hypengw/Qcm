import QtQuick
import QtQuick.Controls

import Qcm.App as QA
import Qcm.Material as MD

MD.GridView {
    id: root
    
    property int space: 8
    property int fixedCellWidth: 160
    readonly property int _width: width - space

    leftMargin: space / 2
    rightMargin: space / 2
    cellHeight: fixedCellWidth + 100
    cellWidth: _width > 0 ? _width / Math.floor((_width / (fixedCellWidth + space / 2))) : 0
}

import QtQuick
import Qcm.Material as MD

MD.GridView {
    id: root

    property int spacing: 12
    property int fixedCellWidth: 160
    readonly property int widthNoMargin: width - leftMargin - rightMargin
    cacheBuffer: flow === GridView.FlowTopToBottom ? cellHeight * 2 : cellWidth * 3

    topMargin: 8
    bottomMargin: 8
    leftMargin: spacing / 2
    rightMargin: spacing / 2
    cellHeight: fixedCellWidth + 100
    cellWidth: widthNoMargin / Math.max(Math.floor(widthNoMargin / (fixedCellWidth + spacing)), 1)

}

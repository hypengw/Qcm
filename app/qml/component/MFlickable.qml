import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QcmApp

Flickable {
    id: root
    bottomMargin: 16
    boundsBehavior: Flickable.StopAtBounds
    clip: true
    contentHeight: contentItem.childrenRect.height
    contentWidth: width - rightMargin - leftMargin
    implicitHeight: contentHeight + topMargin + bottomMargin
    implicitWidth: contentWidth + rightMargin + leftMargin
    leftMargin: 16
    rightMargin: 16
    topMargin: 16

    FlickableScrollHandler {
    }
}

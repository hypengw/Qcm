import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Flickable {
    id: root

    topMargin: 16
    bottomMargin: 16
    rightMargin: 16
    leftMargin: 16
    contentWidth: width - rightMargin - leftMargin
    contentHeight: content.implicitHeight
    implicitHeight: contentHeight + topMargin + bottomMargin
    implicitWidth: contentHeight + rightMargin + leftMargin
    clip: true
    boundsBehavior: Flickable.StopAtBounds
}

import QtQuick
import QtQuick.Controls

import Qcm.App as QA

Flickable {
    id: root
    boundsBehavior: Flickable.StopAtBounds
    clip: true
    contentHeight: contentItem.childrenRect.height
    contentWidth: width - rightMargin - leftMargin
    implicitHeight: contentHeight + topMargin + bottomMargin
    implicitWidth: contentWidth + rightMargin + leftMargin
    leftMargin: 0
    rightMargin: 0
    topMargin: 0
    bottomMargin: 0

    QA.FlickableScrollHandler {
    }
}

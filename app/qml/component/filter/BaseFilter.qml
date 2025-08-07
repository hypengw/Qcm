import QtQuick

Flow {
    id: root
    spacing: 12

    property int condition

    signal clicked
    signal filterChanged

    onConditionChanged: filterChanged()
}

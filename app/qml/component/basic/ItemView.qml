pragma ComponentBehavior: Bound
import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

Item {
    id: root
    property MD.ItemSource source
    property Component delegate
    property Component header
    property int displayMode: QA.Enum.DList
    property int gridSpacing: 12
    property real gridExtraHeight: 0
    property int pageMarginEdges: 0
    readonly property var view: viewLoader.item
    readonly property real contentWidth: view?.contentWidth ?? width
    property string currentKey
    property string anchorKey
    property real anchorY: 0
    property bool ready: false

    function switchView() {
        if (!ready) return;
        if (view) {
            anchorKey = view.keyAtIndex(view.firstVisibleIndex);
            anchorY = view.itemRect(view.firstVisibleIndex).y;
        }
        viewLoader.sourceComponent = displayMode === QA.Enum.DList ? listView : gridView;
    }
    onDisplayModeChanged: switchView()
    Component.onCompleted: { ready = true; switchView(); }

    Loader {
        id: viewLoader
        anchors.fill: parent
        onLoaded: {
            item.currentKey = root.currentKey;
            if (root.anchorKey)
                item.positionAtKey(root.anchorKey, 0, item.topMargin - root.anchorY);
        }
    }
    Connections {
        target: root.view
        function onCurrentChanged() { root.currentKey = root.view.currentKey; }
    }
    Component {
        id: listView
        MD.PageVerticalLazyList {
            source: root.source
            delegate: root.delegate
            header: root.header
            pageMarginEdges: root.pageMarginEdges
            cacheExtent: 300
        }
    }
    Component {
        id: gridView
        MD.PageVerticalLazyGrid {
            source: root.source
            delegate: root.delegate
            header: root.header
            pageMarginEdges: root.pageMarginEdges
            minimumCellExtent: 140
            itemExtent: cellCrossExtent + root.gridExtraHeight
            crossSpacing: root.gridSpacing
            mainSpacing: root.gridSpacing
            cacheExtent: 300
            layoutDirection: Qt.application.layoutDirection
        }
    }
}

pragma ComponentBehavior: Bound
import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

Item {
    id: root

    property var model
    property int displayMode: QA.Enum.DList
    property string type
    property bool busy: false
    property string currentKey
    readonly property var view: viewLoader.item
    readonly property int count: itemSource.count
    readonly property bool scrolling: view?.moving ?? false
    signal activated(var itemId)

    property bool ready: false
    property string anchorKey
    property real anchorY: 0

    function switchView() {
        if (!ready)
            return;
        const component = displayMode === QA.Enum.DList ? listComponent : gridComponent;
        if (viewLoader.sourceComponent === component)
            return;
        if (view) {
            const index = view.firstVisibleIndex;
            anchorKey = view.keyAtIndex(index);
            anchorY = view.itemRect(index).y;
        }
        viewLoader.sourceComponent = component;
    }
    function activate(entry) {
        currentKey = entry.key;
        view.currentKey = currentKey;
        view.forceActiveFocus();
        const itemId = entry.itemId.clone();
        if (type === "artist")
            itemId.type = QA.Enum.ItemArtist;
        activated(itemId);
    }
    function subtitle(entry) {
        if (type === "album")
            return QA.Util.joinName(entry.artists, "/") || qsTr("%1 tracks").arg(entry.trackCount);
        if (type === "mix")
            return qsTr("%1 songs").arg(entry.trackCount);
        return qsTr("%1 albums").arg(entry.albumCount) + " · " + qsTr("%1 songs").arg(entry.musicCount);
    }
    onDisplayModeChanged: switchView()
    Component.onCompleted: {
        ready = true;
        switchView();
    }

    QA.LibraryItemSource {
        id: itemSource
        model: root.model
    }

    MD.Rectangle {
        anchors.fill: parent
        visible: root.displayMode !== QA.Enum.DList
        corners: MD.Util.corners(0, MD.Token.shape.corner.large)
        color: MD.MProp.backgroundColor
    }

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
        function onCurrentChanged() {
            root.currentKey = root.view.currentKey;
        }
        function onItemsNeeded(demand) {
            itemSource.requestMore(demand);
        }
    }

    Column {
        id: footer
        parent: root.view?.contentItem ?? root
        y: root.view?.contentHeight ?? 0
        width: root.width
        topPadding: 8
        bottomPadding: 8
        QA.SyncingLabel {
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    Component {
        id: listComponent
        MD.LazyList {
            source: itemSource
            delegate: listDelegate
            estimatedItemExtent: 72
            cacheExtent: 300
            bottomMargin: root.MD.MProp.page.bottomMargin + footer.height
            synchronousDrag: true
            maximumFlickVelocity: MD.Token.flick.maximumFlickVelocity
            flickDeceleration: MD.Token.flick.flickDeceleration
            MD.NestedScroll.enabled: true
            MD.NestedScroll.axes: Qt.Vertical
            MD.NestedScroll.pressDelay: MD.Token.flick.pressDelay
            MD.ScrollBarBase.vertical: MD.ScrollBar {}
        }
    }
    Component {
        id: gridComponent
        MD.LazyGrid {
            source: itemSource
            delegate: cardDelegate
            minimumCellExtent: 140
            itemExtent: cellCrossExtent + 72
            crossSpacing: 12
            leftMargin: 8
            rightMargin: 8
            bottomMargin: root.MD.MProp.page.bottomMargin + footer.height
            cacheExtent: 300
            synchronousDrag: true
            layoutDirection: Qt.application.layoutDirection
            maximumFlickVelocity: MD.Token.flick.maximumFlickVelocity
            flickDeceleration: MD.Token.flick.flickDeceleration
            MD.NestedScroll.enabled: true
            MD.NestedScroll.axes: Qt.Vertical
            MD.NestedScroll.pressDelay: MD.Token.flick.pressDelay
            MD.ScrollBarBase.vertical: MD.ScrollBar {}
        }
    }
    Component {
        id: cardDelegate
        QA.ImagePlayCard {
            id: card
            required property MD.LazyRow row
            readonly property var entry: row.value
            text: entry.name
            subText: root.subtitle(entry)
            image.source: QA.Util.image_url(entry.itemId)
            picWidth: width
            smallPlayButton: true
            playButtonAlignment: Qt.AlignRight | Qt.AlignBottom
            playAction: root.type === "album" ? albumPlay : null
            mdState.backgroundOpacity: root.displayMode === QA.Enum.DGrid ? 0 : 1
            QA.PlayAction {
                id: albumPlay
                itemId: card.entry.itemId
                icon.name: MD.Token.icon.play_arrow
                checked: true
            }
            onClicked: root.activate(entry)
        }
    }
    Component {
        id: listDelegate
        MD.ListItem {
            id: listItem
            required property MD.LazyRow row
            readonly property var entry: row.value
            index: row.index
            count: root.count
            model: entry
            text: entry.name
            supportText: root.type === "album" ? [QA.Util.joinName(entry.artists, "/"), qsTr("%1 tracks").arg(entry.trackCount)].filter(s => !!s).join(" - ") : root.subtitle(entry)
            maximumLineCount: 2
            radius: MD.Token.shape.corner.large
            corners: MD.Util.corners(0, row.index + 1 === root.count ? radius : 0)
            leader: QA.Image {
                source: QA.Util.image_url(listItem.entry.itemId)
                radius: 8
                displaySize: Qt.size(48, 48)
                implicitWidth: 48
                implicitHeight: 48
            }
            rightPadding: 0
            trailing: MD.IconButton {
                icon.name: MD.Token.icon.more_vert
                onClicked: QA.Action.openItemMenu(listItem.entry.itemId, this)
            }
            onClicked: root.activate(entry)
        }
    }
}

pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.ItemModelSource {
    id: root
    keyMode: MD.ItemModelSource.PersistentIndexKey
    readonly property Connections fetchStateObserver: Connections {
        target: root.model
        ignoreUnknownSignals: true
        function onHasMoreChanged() { root.notifyFetchStateChanged(); }
    }
}

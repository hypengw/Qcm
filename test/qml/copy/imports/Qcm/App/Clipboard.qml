pragma Singleton
import QtQuick
QtObject {
    property string text: ""
    property bool supported: true
    property bool fail: false
    property int writes: 0
    function copyText(value: string): bool {
        if (!supported || fail || !value.trim().length)
            return false;
        text = value;
        writes++;
        return true;
    }
}

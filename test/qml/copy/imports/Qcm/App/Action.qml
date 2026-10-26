pragma Singleton
import QtQuick
QtObject {
    property var messages: []
    function toast(text, duration) {
        messages = messages.concat([text]);
    }
}

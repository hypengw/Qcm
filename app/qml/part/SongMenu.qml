import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QcmApp
import ".."
import "../component"

MMenu {
    id: root

    required property t_song song

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    Action {
        enabled: root.song.itemId !== QA.cur_song.itemId
        icon.name: Theme.ic.play_arrow
        text: qsTr('play next')

        onTriggered: {
            QA.playlist.appendNext(root.song);
        }
    }
    Action {
        icon.name: Theme.ic.album
        text: qsTr('go to album')

        onTriggered: {
            QA.route(root.song.album.itemId);
        }
    }
    Action {
        icon.name: Theme.ic.person
        text: qsTr('go to artist')

        onTriggered: {
            const artists = root.song.artists;
            if (artists.length === 1)
                QA.route(artists[0].itemId);
            else
                QA.show_popup('qrc:/QcmApp/qml/part/ArtistsPopup.qml', {
                        "model": artists
                    });
        }
    }
    Action {
        icon.name: Theme.ic.link
        text: qsTr('copy url')

        onTriggered: {
            Clipboard.text = song.itemId.url();
        }
    }
}

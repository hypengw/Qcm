import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Menu {
    id: root

    required property QA.t_song song

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    Action {
        enabled: root.song.itemId !== QA.Global.cur_song.itemId
        icon.name: MD.Token.icon.play_arrow
        text: qsTr('play next')

        onTriggered: {
            QA.Global.playlist.appendNext(root.song);
        }
    }
    Action {
        icon.name: MD.Token.icon.album
        text: qsTr('go to album')

        onTriggered: {
            QA.Global.route(root.song.album.itemId);
        }
    }
    Action {
        icon.name: MD.Token.icon.person
        text: qsTr('go to artist')

        onTriggered: {
            const artists = root.song.artists;
            if (artists.length === 1)
                QA.Global.route(artists[0].itemId);
            else
                QA.Global.show_page_popup('qrc:/Qcm/App/qml/component/ArtistsPopup.qml', {
                        "model": artists
                    });
        }
    }
    Action {
        icon.name: MD.Token.icon.comment
        text: qsTr('commnet')
        onTriggered: {
            QA.Global.show_page_popup('qrc:/Qcm/App/qml/page/CommentPage.qml', {
                    "itemId": root.song.itemId
                });
        }
    }
    Action {
        icon.name: MD.Token.icon.link
        text: qsTr('copy url')

        onTriggered: {
            QA.Clipboard.text = song.itemId.url();
        }
    }
}

pragma Singleton
import QtQml
import QtQuick
import QtMultimedia
import Qt.labs.settings
import QcmApp

Item {
    property QtObject main_win: null
    readonly property string title: 'Qcm'
    readonly property alias user_info: m_querier_user.data
    readonly property bool is_login: m_querier_user.data.userId.valid()
    readonly property string loop_icon: switch (m_playlist.loopMode) {
    case Playlist.SingleLoop:
        return Theme.ic.repeat_one;
    case Playlist.ListLoop:
        return Theme.ic.loop;
    case Playlist.ShuffleLoop:
        return Theme.ic.shuffle;
    case Playlist.NoneLoop:
    default:
        return Theme.ic.trending_flat;
    }
    property alias querier_song: m_querier_song
    property alias querier_user: m_querier_user
    property alias player: m_player
    property alias playlist: m_playlist
    readonly property t_song cur_song: m_playlist.cur

    function join_name(objs, split) {
        const names = objs.map((o) => {
            return o.name;
        });
        return names.join(split);
    }

    function create_item(url, props, parent) {
        const com = Qt.createComponent(url);
        if (com.status === Component.Ready) {
            try {
                return com.createObject(parent, props);
            } catch (e) {
                console.error(e);
            }
        } else if (com.status == Component.Error) {
            console.error(com.errorString());
        }
    }

    function item_id_url(itemId) {
        switch (itemId.type) {
        case ItemId.Album:
            return 'qrc:/QcmApp/qml/page/AlbumDetailPage.qml';
        case ItemId.Playlist:
            return 'qrc:/QcmApp/qml/page/PlaylistDetailPage.qml';
        case ItemId.Artist:
            return 'qrc:/QcmApp/qml/page/ArtistDetailPage.qml';
        }
        return '';
    }

    function show_popup(url, props) {
        const popup = create_item(url, props, main_win);
        popup.closed.connect(() => {
            popup.destroy(1000);
        });
        popup.open();
    }

    function route(dest) {
        let url = dest;
        let props = {
        };
        switch (dest.type) {
        case ItemId.Album:
            url = 'qrc:/QcmApp/qml/page/AlbumDetailPage.qml';
            props = {
                "itemId": dest
            };
            break;
        case ItemId.Playlist:
            url = 'qrc:/QcmApp/qml/page/PlaylistDetailPage.qml';
            props = {
                "itemId": dest
            };
            break;
        case ItemId.Artist:
            url = 'qrc:/QcmApp/qml/page/ArtistDetailPage.qml';
            props = {
                "itemId": dest
            };
            break;
        }
        main_win.push_page(url, props);
    }

    Settings {
        property alias loop: m_playlist.loopMode

        category: 'play'
    }

    Playlist {
        id: m_playlist

        onCurChanged: function(refresh) {
            if (refresh)
                m_querier_song.ids = [];

            if (cur.itemId.valid())
                m_querier_song.ids = [cur.itemId.sid];

        }
    }

    UserAccountQuerier {
        id: m_querier_user

        onStatusChanged: {
            if (status === ApiQuerierBase.Finished)
                App.loginPost(data);

        }
    }

    SongUrlQuerier {
        id: m_querier_song

        autoReload: ids.length > 0
    }

    MediaPlayer {
        id: m_player

        readonly property bool playing: {
            switch (playbackState) {
            case MediaPlayer.PlayingState:
                return true;
            default:
                return false;
            }
        }

        function setPos(pos) {
            position = pos * duration;
        }

        source: {
            const songs = m_querier_song.data.songs;
            if (songs.length)
                return songs[0].url;

            return '';
        }
        onSourceChanged: (source) => {
            if (source)
                play();

        }
        onPlaybackStateChanged: {
            if (playbackState === MediaPlayer.StoppedState) {
                if (position / duration > 0.99)
                    m_playlist.next();

            }
        }

        audioOutput: AudioOutput {
        }

    }

}

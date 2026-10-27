import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

Item {
    id: root
    parent: testWindow?.contentItem ?? null
    readonly property QA.CommentStore commentStore: store
    property int state: 0
    property int frames: 0
    property bool captured: false
    property string captureName: ""
    function findItem(item, name) {
        if (item.objectName === name) return item;
        for (const child of item.children) {
            const found = findItem(child, name);
            if (found) return found;
        }
        return null;
    }
    function check(value, message) {
        if (!value) { console.error('comments test: ' + message); Qt.exit(1); throw new Error(message); }
    }
    QA.CommentStore { id: store }
    QA.CommentsQuery {
        id: query
        itemId: testItemId
        store: root.commentStore
        Component.onCompleted: delayReload()
    }
    QA.CommentsQuery {
        id: replies
        itemId: testItemId
        parentId: '1'
        store: root.commentStore
    }
    QA.CommentPopup { id: popup; itemId: testItemId }
    Timer {
        interval: 50
        repeat: true
        running: true
        onTriggered: {
            switch (root.state) {
            case 0:
                if (!query.loaded) return;
                root.check(query.supported && query.model.count === 2, 'first page');
                root.check(store.view('1').value.hasLiked && !store.view('1').value.liked, 'known false');
                query.fetchMore(); root.state++; break;
            case 1:
                if (query.querying) return;
                root.check(query.model.count === 3 && !query.hasMore, 'deduplicated continuation');
                store.setLiked('1', true); store.setLiked('1', true); root.state++; break;
            case 2:
                if (store.view('1').pending) return;
                root.check(store.view('1').value.liked && store.view('1').value.likeCount === 1, 'confirmed reaction');
                replies.reload(); root.state++; break;
            case 3:
                if (!replies.loaded) return;
                root.check(replies.model.count === 1 && store.view('1').value.liked, 'shared replies');
                store.setLiked('1', false); root.state++; break;
            case 4:
                if (store.view('1').pending) return;
                root.check(!store.view('1').value.liked && store.view('1').value.likeCount === 0, 'unlike');
                store.setLiked('2', true); root.state++; break;
            case 5:
                if (store.view('2').pending || query.querying || store.view('2').uncertain) return;
                root.check(!store.view('2').value.liked, 'failed reaction retained');
                query.order = 1; root.state++; break;
            case 6:
                if (!query.loaded || query.querying) return;
                root.check(store.view('1').value.content.startsWith('Hot:'), 'order switch');
                testWindow.width = 900; testWindow.height = 720;
                popup.requestOpen(); root.state++; break;
            case 7:
                if (!popup.opened || ++root.frames < 8) return;
                const list = findItem(popup.contentItem.currentItem, 'commentsList');
                if (!list || list.count !== 2 || !list.itemAtIndex(0)) return;
                root.check(list.width > 0 && list.height > 0 && list.itemAtIndex(0).height > 0, 'popup comment geometry');
                root.frames = 0;
                root.captureName = captureBase + '-desktop.png';
                root.state++; break;
            case 8:
                if (!root.captured) return;
                popup.contentItem.currentItem.openReplies('1'); root.state++; break;
            case 9:
                if (popup.contentItem.depth !== 2 || ++root.frames < 8) return;
                popup.contentItem.popCurrentItem();
                testWindow.width = 360; testWindow.height = 640;
                root.frames = 0; root.captured = false; root.state++; break;
            case 10:
                if (popup.contentItem.depth !== 1 || ++root.frames < 8) return;
                root.captureName = captureBase + '-compact.png';
                root.state++; break;
            case 11:
                if (!root.captured) return;
                popup.close(); store.close();
                console.log('comments ui checks passed');
                Qt.exit(0); break;
            }
        }
    }
}

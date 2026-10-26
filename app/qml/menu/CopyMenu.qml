pragma ComponentBehavior: Bound
import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Menu {
    id: root
    required property QA.item_id itemId
    readonly property QA.EntityView entity: QA.Store.item(itemId)
    readonly property string name: entity?.name ?? ""
    readonly property string artists: (entity?.artists ?? []).map(artist => artist.name).filter(name => name.trim().length > 0).join(" / ")
    readonly property string album: entity?.album?.name ?? ""
    readonly property bool isSong: itemId.type === QA.Enum.ItemSong
    readonly property string information: {
        if (!name.trim().length)
            return "";
        const title = artists.length ? `${name} - ${artists}` : name;
        return album.length ? `${title}\n${qsTr("Album")}: ${album}` : title;
    }

    title: qsTr("Copy")

    QA.CopyAction {
        text: qsTr("Title")
        icon.name: MD.Token.icon.title
        copyText: root.name
        onCopied: root.dismiss()
    }
    QA.CopyAction {
        text: qsTr("Artists")
        icon.name: MD.Token.icon.person
        visible: root.isSong || root.itemId.type === QA.Enum.ItemAlbum
        copyText: root.artists
        onCopied: root.dismiss()
    }
    QA.CopyAction {
        text: qsTr("Album")
        icon.name: MD.Token.icon.album
        visible: root.isSong
        copyText: root.album
        onCopied: root.dismiss()
    }
    QA.CopyAction {
        text: qsTr("Song information")
        icon.name: MD.Token.icon.description
        visible: root.isSong
        copyText: root.information
        onCopied: root.dismiss()
    }
    QA.CopyAction {
        text: qsTr("Link")
        icon.name: MD.Token.icon.link
        copyText: m_link.status === QA.Enum.Finished ? m_link.data.url : ""
        busy: m_link.querying
        onCopied: root.dismiss()
        tooltip: m_link.querying ? qsTr("Loading share link") : m_link.error || (copyText.length ? "" : qsTr("This item has no public share link"))
    }
    QA.ShareLinkQuery {
        id: m_link
        itemId: root.itemId
        Component.onCompleted: delayReload()
    }
}

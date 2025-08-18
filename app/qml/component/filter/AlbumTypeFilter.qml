pragma ComponentBehavior: Bound
import QtQuick
import Qcm.App as QA
import Qcm.Msg as QM
import Qcm.Material as MD

QA.TypeFilter {
    id: root
    name: qsTr('type')

    function format(t: int): string {
        switch (t) {
        case QM.AlbumType.ALBUM_TYPE_ALBUM:
            return qsTr('Album');
        case QM.AlbumType.ALBUM_TYPE_SINGLE:
            return qsTr('Single');
        case QM.AlbumType.ALBUM_TYPE_EP:
            return qsTr('EP');
        case QM.AlbumType.ALBUM_TYPE_COMPILATION:
            return qsTr('Compilation');
        case QM.AlbumType.ALBUM_TYPE_SOUNDTRACK:
            return qsTr('Soundtrack');
        case QM.AlbumType.ALBUM_TYPE_LIVE:
            return qsTr('Live');
        default:
            return 'Unknown';
        }
    }

    MD.InputChip {
        id: m_value
        visible: root.condition !== QM.TypeCondition.TYPE_CONDITION_UNSPECIFIED
        text: root.format(root.value)
        onClicked: {
            QA.Action.openPopup(m_menu_comp);
        }
    }

    Component {
        id: m_menu_comp
        QA.EnumMenu {
            parent: m_value
            y: m_value.y + m_value.height
            model: [QM.AlbumType.ALBUM_TYPE_ALBUM, QM.AlbumType.ALBUM_TYPE_SINGLE, QM.AlbumType.ALBUM_TYPE_EP, QM.AlbumType.ALBUM_TYPE_COMPILATION, QM.AlbumType.ALBUM_TYPE_SOUNDTRACK, QM.AlbumType.ALBUM_TYPE_LIVE, QM.AlbumType.ALBUM_TYPE_UNSPECIFIED].map(t => {
                return {
                    type: t,
                    name: root.format(t)
                };
            })
            onSelected: type => {
                root.value = type;
                close();
            }
        }
    }
}

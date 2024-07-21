import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Menu {
    id: root

    required property QA.t_program program

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    QA.PlaynextAction {
        song: root.program.song
    }
    QA.CommentAction {
        itemId: root.program.itemId
    }
}
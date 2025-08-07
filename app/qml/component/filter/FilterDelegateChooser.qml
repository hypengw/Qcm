import QtQml.Models
import QtQuick
import Qcm.Msg as QM

DelegateChooser {
    id: root
    role: "type"
    DelegateChoice {
        roleValue: QM.FilterType.FILTER_TYPE_TITLE
    }
    DelegateChoice {
        roleValue: QM.FilterType.FILTER_TYPE_TRACK_COUNT
    }
}

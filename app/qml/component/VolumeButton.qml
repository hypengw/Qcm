import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.IconButton {
    id: control
    property real volume: 0
    icon.name: volume > 0.5 ? MD.Token.icon.volume_up : (QA.Global.player.volume == 0.0 ? MD.Token.icon.volume_mute : MD.Token.icon.volume_down)

    signal volumeSeted(volume: real)

    Component {
        id: comp_popup
        MD.Popup {
            dim: false
            modal: true
            height: 120
            width: slider.handle.width * 1.5
            property alias value: slider.value
            contentItem: MD.Slider {
                id: slider
                orientation: Qt.Vertical
                signal volumeSeted(volume: real)
                onMoved: {
                    volumeSeted(value);
                }
            }
        }
    }

    onClicked: {
        const popup = MD.Util.show_popup(comp_popup, {
            "value": control.volume,
            "y": 0
        }, this);
        popup.x = (control.width - popup.width) / 2.0;
        popup.y = -popup.height;
        const slider = popup.contentItem;
        popup.contentItem.volumeSeted.connect(control.volumeSeted);
    }
}

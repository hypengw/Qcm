import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.Slider {
    readonly property real byteValue: value > 9 ? (value - 9) * m_GB : value * m_MB
    readonly property int m_GB: Math.pow(2, 30)
    readonly property int m_MB: 100 * Math.pow(2, 20)

    function setByteValue(v) {
        value = v >= m_GB ? v / m_GB + 9 : v / m_MB;
    }

    snapMode: T.Slider.SnapAlways
}

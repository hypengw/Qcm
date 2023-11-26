import Qcm.App as QA
import Qcm.Material as MD

MD.Image {
    property size displaySize

    sourceSize: QA.App.image_size(displaySize, QA.Global.cover_quality, this)
}

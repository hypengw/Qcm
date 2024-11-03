import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

import "../js/util.mjs" as Util

MD.Image {
    property size displaySize
    property bool fixedSize: true

    sourceSize: {
        if(fixedSize) {
            return displaySize;
        } else {
            return QA.App.bound_image_size(displaySize);
        }
        // QA.App.image_size(displaySize, QA.Global.cover_quality, this)
    }
}

#include "qml_material/util.h"

#include "core/log.h"
#include <set>

namespace qml_material
{
CornersGroup Util::corner(QVariant in) {
    CornersGroup out;
    if (in.canConvert<qreal>()) {
        out = CornersGroup(in.value<qreal>());
    } else if (auto list = in.toList(); ! list.empty()) {
        switch (list.size()) {
        case 1: {
            out = CornersGroup(list[0].value<qreal>());
            break;
        }
        case 2: {
            out.setTopLeft(list[0].value<qreal>());
            out.setTopRight(list[0].value<qreal>());
            out.setBottomLeft(list[1].value<qreal>());
            out.setBottomRight(list[1].value<qreal>());
            break;
        }
        case 3: {
            out.setTopLeft(list[0].value<qreal>());
            out.setTopRight(list[1].value<qreal>());
            out.setBottomLeft(list[2].value<qreal>());
            out.setBottomRight(list[1].value<qreal>());
            break;
        }
        default:
        case 4: {
            out.setTopLeft(list[0].value<qreal>());
            out.setTopRight(list[1].value<qreal>());
            out.setBottomLeft(list[2].value<qreal>());
            out.setBottomRight(list[3].value<qreal>());
        }
        }
    }
    return out;
}

CornersGroup Util::corner(qreal br, qreal tr, qreal bl, qreal tl) {
    return CornersGroup(br, tr, bl, tl);
}

void Util::track(QVariant, Track t) {
    switch (t) {
    case TrackCreate:
        m_tracked++;
        WARN_LOG("track create {}", m_tracked);
        break;
    case TrackDelete:
        m_tracked--;
        WARN_LOG("track delete {}", m_tracked);
        break;
    }
}

} // namespace qml_material
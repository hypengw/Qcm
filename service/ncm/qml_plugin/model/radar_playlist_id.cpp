#include "service_qml_ncm/model/radar_playlist_id.h"
#include "service_qml_ncm/model.h"

namespace ncm::qml
{

namespace
{
constexpr std::array ids {
    "3136952023", // 私人
    "5320167908", // 时光
    "5362359247", // 宝藏
    "5327906368", // 乐迷
    "5300458264", // 新歌
    "5341776086", // 神秘
    "9102252203", // 每周
    "9380018200", // 曾经
};
}

int      RadarPlaylistIdModel::rowCount(const QModelIndex&) const { return ids.size(); }
QVariant RadarPlaylistIdModel::data(const QModelIndex& index, int role) const {
    auto row = index.row();
    if (row < rowCount()) {
        switch (role) {
        case Qt::UserRole: {
            return QVariant::fromValue(ncm::to_ncm_id(ncm::model::IdType::Playlist, ids.at(row)));
        }
        }
    }
    return {};
}
QHash<int, QByteArray> RadarPlaylistIdModel::roleNames() const {
    return { { Qt::UserRole, "id" } };
}

} // namespace ncm::qml
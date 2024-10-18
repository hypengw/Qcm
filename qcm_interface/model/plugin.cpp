#include "qcm_interface/model/plugin_model.h"
#include "qcm_interface/global.h"
#include "qcm_interface/plugin_p.h"
#include <QHash>

namespace qcm
{

class PluginModel::Private {
public:
    Private(): plugins(PluginManager::instance()->d_func()->plugins) {}
    ~Private() {}
    decltype(PluginManager::Private::plugins)& plugins;
};

PluginModel::PluginModel(QObject* parent): QAbstractListModel(parent), d_ptr(make_up<Private>()) {}
PluginModel::~PluginModel() {}

int PluginModel::rowCount(const QModelIndex&) const {
    C_D(const PluginModel);
    return d->plugins.size();
}
QVariant PluginModel::data(const QModelIndex& index, int role) const {
    C_D(const PluginModel);
    auto row = index.row();
    auto n   = rowCount();
    if (row >= n) return {};

    auto it = d->plugins.begin();
    std::advance(it, row);
    auto& p = *it;

    switch (role) {
    case Qt::UserRole + 1: {
        return QVariant::fromValue(p.second->info());
    }
    case Qt::UserRole + 2: {
        return QVariant::fromValue(p.second->router());
    }
    }
    return {};
}

QHash<int, QByteArray> PluginModel::roleNames() const {
    return { { Qt::UserRole + 1, "info" }, { Qt::UserRole + 2, "router" } };
}
} // namespace qcm
#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>
#include <QMap>

#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/playlist_catalogue.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
struct PlaylistCatalogueItem {
    Q_GADGET
public:
    GATGET_PROPERTY(QString, name, name)
    GATGET_PROPERTY(i64, resourceCount, resourceCount)
    GATGET_PROPERTY(QString, category, category)
    GATGET_PROPERTY(bool, hot, hot)
};
} // namespace model
} // namespace qcm

template<>
struct To<qcm::model::PlaylistCatalogueItem> {
    static auto from(const ncm::model::PlaylistCatalogue& in) {
        qcm::model::PlaylistCatalogueItem o;
        convert(o.name, in.name);
        convert(o.resourceCount, in.resourceCount);
        convert(o.category, in.category);
        convert(o.hot, in.hot);
        return o;
    }
};

namespace qcm
{
namespace model
{
class PlaylistCatalogue : public QObject {
    Q_OBJECT
public:
    PlaylistCatalogue(QObject* parent = nullptr): QObject(parent) {}
    using out_type = ncm::api_model::PlaylistCatalogue;

    Q_PROPERTY(QList<QString> categories READ categories NOTIFY infoChanged)

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;
        for (auto el : in.sub) {
            auto cat_id = To<std::string>::from(el.category);
            if (! in.categories.contains(cat_id)) continue;
            auto cat = To<QString>::from(in.categories.at(cat_id));
            if (! m_cats.contains(cat)) m_cats[cat] = {};
            m_cats[cat].emplace_back(To<PlaylistCatalogueItem>::from(el));
        }
        emit infoChanged();
    }

    auto categories() const { return m_cats.keys(); }

    Q_INVOKABLE std::vector<PlaylistCatalogueItem> category(const QString& cat) {
        if (m_cats.contains(cat)) {
            return m_cats[cat];
        }
        return {};
    }

signals:
    void infoChanged();

private:
    QMap<QString, std::vector<PlaylistCatalogueItem>> m_cats;
};
static_assert(modelable<PlaylistCatalogue, ncm::api::PlaylistCatalogue>);

} // namespace model

using PlaylistCatalogueQuerier_base =
    ApiQuerier<ncm::api::PlaylistCatalogue, model::PlaylistCatalogue>;
class PlaylistCatalogueQuerier : public PlaylistCatalogueQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    PlaylistCatalogueQuerier(QObject* parent = nullptr): PlaylistCatalogueQuerier_base(parent) {}
};

} // namespace qcm

#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/cloudsearch.h"

#include "meta_model/qgadgetlistmodel.h"
#include "meta_model/qvariantlistmodel.h"

#include "core/log.h"

namespace ncm::qml
{
namespace model
{

class CloudSearch
    : public meta_model::VariantListModel<qcm::model::Song, qcm::model::Album, qcm::model::Mix,
                                          qcm::model::Artist, qcm::model::Radio> {
    Q_OBJECT
public:
    CloudSearch(QObject* parent = nullptr)
        : meta_model::VariantListModel<qcm::model::Song, qcm::model::Album, qcm::model::Mix,
                                       qcm::model::Artist, qcm::model::Radio>(parent),
          m_has_more(true) {
        connect(this, &CloudSearch::modelReset, this, [this]() {
            fetchMore({});
        });
    }

    using out_type = ncm::api_model::CloudSearch;

    template<typename T, typename D>
    class Helper {
    public:
        Helper(const CloudSearch& p, const out_type& re): p(p), re(re) {}

        operator bool() const {
            return std::holds_alternative<T>(re.result) && p.holds_alternative<D>();
        }

        const auto& src() { return std::get<T>(re.result); }

        template<typename Tin>
        auto to(const Tin& in) -> std::vector<D> {
            return convert_from<std::vector<D>>(in);
        }

    private:
        const CloudSearch& p;
        const out_type&    re;
    };

    void handle_output(const out_type& re, const auto&) {
        {
            Helper<out_type::SongResult, qcm::model::Song> h(*this, re);
            if (h) {
                this->insert(rowCount(), h.to(helper::value_or_default(h.src().songs)));
                m_has_more = h.src().songCount > rowCount();
            }
        }
        {
            Helper<out_type::AlbumResult, qcm::model::Album> h(*this, re);
            if (h) {
                this->insert(rowCount(), h.to(helper::value_or_default(h.src().albums)));
                m_has_more = h.src().albumCount > rowCount();
            }
        }
        {
            Helper<out_type::PlaylistResult, qcm::model::Mix> h(*this, re);
            if (h) {
                this->insert(rowCount(), h.to(helper::value_or_default(h.src().playlists)));
                m_has_more = h.src().playlistCount > rowCount();
            }
        }
        {
            Helper<out_type::ArtistResult, qcm::model::Artist> h(*this, re);
            if (h) {
                this->insert(rowCount(), h.to(helper::value_or_default(h.src().artists)));
                m_has_more = h.src().artistCount > rowCount();
            }
        }
        {
            Helper<out_type::DjradioResult, qcm::model::Radio> h(*this, re);
            if (h) {
                this->insert(rowCount(), h.to(helper::value_or_default(h.src().djRadios)));
                m_has_more = h.src().djRadiosCount > rowCount();
            }
        }
    }

    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }

    void updateType(int);
signals:
    void fetchMoreReq(qint32);

private:
    bool m_has_more;
};
} // namespace model

using CloudSearchQuerier_base = NcmApiQuery<ncm::api::CloudSearch, model::CloudSearch>;
class CloudSearchQuerier : public CloudSearchQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    CloudSearchQuerier(QObject* parent = nullptr): CloudSearchQuerier_base(parent) {
        connect(
            this,
            &CloudSearchQuerier::changed_keywords,
            this,
            [this]() {
                tdata()->resetModel();
            },
            Qt::DirectConnection);
        connect(
            this,
            &CloudSearchQuerier::changed_type,
            this,
            [this]() {
                tdata()->updateType(api().input.type);
            },
            Qt::DirectConnection);
        emit changed_type();
    }

    enum Type
    {
        SongType     = 1,
        AlbumType    = 10,
        AritstType   = 100,
        PlaylistType = 1000,
        DjradioType  = 1009
        // 1: 单曲, 10: 专辑, 100: 歌手, 1000: 歌单, 1002: 用户, 1004: MV, 1006: 歌词,
        // 1009: 电台, 1014: 视频
    };
    Q_ENUM(Type)

    FORWARD_PROPERTY(QString, keywords, keywords)
    FORWARD_PROPERTY(int, type, type)
    FORWARD_PROPERTY(qint32, offset, offset)
    FORWARD_PROPERTY(qint32, limit, limit)

public:
    void fetch_more(qint32 cur_count) override { set_offset(cur_count); }
};

} // namespace ncm::qml

inline void ncm::qml::model::CloudSearch::updateType(int t) {
    const QMetaObject* meta { nullptr };
    std::size_t        i { 0 };
    switch (t) {
    case CloudSearchQuerier::AlbumType:
        meta = &qcm::model::Album::staticMetaObject;
        i    = 1;
        break;
    case CloudSearchQuerier::PlaylistType:
        meta = &qcm::model::Mix::staticMetaObject;
        i    = 2;
        break;
    case CloudSearchQuerier::AritstType:
        meta = &qcm::model::Artist::staticMetaObject;
        i    = 3;
        break;
    case CloudSearchQuerier::DjradioType:
        meta = &qcm::model::Radio::staticMetaObject;
        i    = 4;
        break;
    default:
    case CloudSearchQuerier::SongType:
        meta = &qcm::model::Song::staticMetaObject;
        i    = 0;
        break;
    }
    this->updateRoleNames(*meta, i);
}
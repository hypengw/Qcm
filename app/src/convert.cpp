#include "Qcm/app.hpp"
#include "Qcm/message/filter.qpb.h"
#include "kstore/qt/meta_utils.hpp"

#include <QtCore/QJsonValue>
#include <QtCore/QJsonObject>
#include <QtCore/QVariant>

namespace qcm
{

namespace
{
auto albumfilter_to_json(const msg::filter::AlbumFilter& f) -> QJsonObject {
    auto obj = QJsonObject();

    using M = msg::filter::AlbumFilter::PayloadFields;
    switch (f.payloadField()) {
    case M::UninitializedField: {
        break;
    }
    case M::TitleFilter: {
        auto val = kstore::qvariant_to_josn(QVariant::fromValue(f.titleFilter()));
        obj.insert("titleFilter", val);
        break;
    }
    case M::TrackFilter: {
        auto val = kstore::qvariant_to_josn(QVariant::fromValue(f.trackFilter()));
        obj.insert("trackFilter", val);
        break;
    }
    case M::ArtistNameFilter: {
        auto val = kstore::qvariant_to_josn(QVariant::fromValue(f.artistNameFilter()));
        obj.insert("artistNameFilter", val);
        break;
    }
    case M::ArtistIdFilter: {
        auto val = kstore::qvariant_to_josn(QVariant::fromValue(f.artistIdFilter()));
        obj.insert("artistIdFilter", val);
        break;
    }
    case M::AlbumArtistIdFilter: {
        auto val = kstore::qvariant_to_josn(QVariant::fromValue(f.albumArtistIdFilter()));
        obj.insert("albumArtistIdFilter", val);
        break;
    }
    case M::YearFilter: {
        auto val = kstore::qvariant_to_josn(QVariant::fromValue(f.yearFilter()));
        obj.insert("yearFilter", val);
        break;
    }
    case M::DurationFilter: {
        auto val = kstore::qvariant_to_josn(QVariant::fromValue(f.durationFilter()));
        obj.insert("durationFilter", val);
        break;
    }
    default: {
        qWarning() << "Unknown AlbumFilter payload field: " << f.payloadField();
        break;
    }
    }

    return obj;
}

auto albumfilter_from_json(const QJsonObject& obj) -> msg::filter::AlbumFilter {
    auto f = msg::filter::AlbumFilter();

    if (auto jval = obj.value("titleFilter"); jval.isObject()) {
        if (auto val = kstore::qvariant_from_josn<msg::filter::TitleFilter>(jval)) {
            f.setType(msg::FilterTraits<std::decay_t<decltype(*val)>>::type);
            f.setTitleFilter(*val);
        }
    } else if (auto jval = obj.value("trackFilter"); jval.isObject()) {
        if (auto val = kstore::qvariant_from_josn<msg::filter::TrackCountFilter>(jval)) {
            f.setType(msg::FilterTraits<std::decay_t<decltype(*val)>>::type);
            f.setTrackFilter(*val);
        }
    } else if (auto jval = obj.value("artistNameFilter"); jval.isObject()) {
        if (auto val = kstore::qvariant_from_josn<msg::filter::ArtistNameFilter>(jval)) {
            f.setType(msg::FilterTraits<std::decay_t<decltype(*val)>>::type);
            f.setArtistNameFilter(*val);
        }
    } else if (auto jval = obj.value("artistIdFilter"); jval.isObject()) {
        if (auto val = kstore::qvariant_from_josn<msg::filter::ArtistIdFilter>(jval)) {
            f.setType(msg::FilterTraits<std::decay_t<decltype(*val)>>::type);
            f.setArtistIdFilter(*val);
        }
    } else if (auto jval = obj.value("albumArtistIdFilter"); jval.isObject()) {
        if (auto val = kstore::qvariant_from_josn<msg::filter::AlbumArtistIdFilter>(jval)) {
            f.setType(msg::FilterTraits<std::decay_t<decltype(*val)>>::type);
            f.setAlbumArtistIdFilter(*val);
        }
    } else if (auto jval = obj.value("yearFilter"); jval.isObject()) {
        if (auto val = kstore::qvariant_from_josn<msg::filter::YearFilter>(jval)) {
            f.setType(msg::FilterTraits<std::decay_t<decltype(*val)>>::type);
            f.setYearFilter(*val);
        }
    } else if (auto jval = obj.value("durationFilter"); jval.isObject()) {
        if (auto val = kstore::qvariant_from_josn<msg::filter::DurationFilter>(jval)) {
            f.setType(msg::FilterTraits<std::decay_t<decltype(*val)>>::type);
            f.setDurationFilter(*val);
        }
    } else {
        qWarning() << "Unknown AlbumFilter payload field in JSON object" << obj;
    }

    return f;
}

auto artistfilter_to_json(const msg::filter::ArtistFilter& f) -> QJsonObject {
    auto obj = QJsonObject();

    using M = msg::filter::ArtistFilter::PayloadFields;
    switch (f.payloadField()) {
    case M::UninitializedField: {
        break;
    }
    case M::NameFilter: {
        auto val = kstore::qvariant_to_josn(QVariant::fromValue(f.nameFilter()));
        obj.insert("nameFilter", val);
        break;
    }
    case M::YearFilter: {
        auto val = kstore::qvariant_to_josn(QVariant::fromValue(f.yearFilter()));
        obj.insert("yearFilter", val);
        break;
    }
    case M::AlbumTitleFilter: {
        auto val = kstore::qvariant_to_josn(QVariant::fromValue(f.albumTitleFilter()));
        obj.insert("albumTitleFilter", val);
        break;
    }
    case M::AddedDateFilter: {
        auto val = kstore::qvariant_to_josn(QVariant::fromValue(f.addedDateFilter()));
        obj.insert("addedDateFilter", val);
        break;
    }
    default: {
        qWarning() << "Unknown ArtistFilter payload field: " << f.payloadField();
        break;
    }
    }

    return obj;
}
auto artistfilter_from_json(const QJsonObject& obj) -> msg::filter::ArtistFilter {
    auto f = msg::filter::ArtistFilter();

    if (auto jval = obj.value("nameFilter"); jval.isObject()) {
        if (auto val = kstore::qvariant_from_josn<msg::filter::NameFilter>(jval)) {
            f.setType(msg::FilterTraits<std::decay_t<decltype(*val)>>::type);
            f.setNameFilter(*val);
        }
    } else if (auto jval = obj.value("yearFilter"); jval.isObject()) {
        if (auto val = kstore::qvariant_from_josn<msg::filter::YearFilter>(jval)) {
            f.setType(msg::FilterTraits<std::decay_t<decltype(*val)>>::type);
            f.setYearFilter(*val);
        }
    } else if (auto jval = obj.value("albumTitleFilter"); jval.isObject()) {
        if (auto val = kstore::qvariant_from_josn<msg::filter::AlbumTitleFilter>(jval)) {
            f.setType(msg::FilterTraits<std::decay_t<decltype(*val)>>::type);
            f.setAlbumTitleFilter(*val);
        }
    } else if (auto jval = obj.value("addedDateFilter"); jval.isObject()) {
        if (auto val = kstore::qvariant_from_josn<msg::filter::AddedDateFilter>(jval)) {
            f.setType(msg::FilterTraits<std::decay_t<decltype(*val)>>::type);
            f.setAddedDateFilter(*val);
        }
    } else {
        qWarning() << "Unknown AlbumFilter payload field in JSON object" << obj;
    }

    return f;
}
} // namespace

void App::register_converters() {
    QMetaType::registerConverter<msg::filter::AlbumFilter, QJsonObject>(albumfilter_to_json);
    QMetaType::registerConverter<QJsonObject, msg::filter::AlbumFilter>(albumfilter_from_json);

    QMetaType::registerConverter<msg::filter::ArtistFilter, QJsonObject>(artistfilter_to_json);
    QMetaType::registerConverter<QJsonObject, msg::filter::ArtistFilter>(artistfilter_from_json);

    QMetaType::registerConverter<QtProtobuf::int32, QJsonValue>([](auto i) {
        return QJsonValue(i);
    });
    QMetaType::registerConverter<QtProtobuf::int64, QJsonValue>([](auto i) {
        return QJsonValue((qint64)i);
    });
} // namespace
} // namespace qcm
module;
#include <rstd/enum.hpp>
export module qcm.backend:api.recommendations;
import :identity;
export import :protocol.message;
export import :error;
export import :provider.runtime;
import :provider.item;
import :api.provider;
import :model.album;
import :model.artist;
import :model.home;
import :model.song;
import :db.query.home;
import :db.error.process;
import :db.sync.run;
import :db.database;
import :convert;
export import rstd;
using rstd::collections::HashMap;
using namespace rstd::prelude;
using namespace rstd::literals;
using HomeRadio          = p_qcm::p_msg::p_model::M_RadioQueue;
using HomeCard           = p_qcm::p_msg::p_model::M_ForYouCard;
using HomeRecommendation = p_qcm::p_msg::p_model::M_HomeRecommendation;

namespace qcm::backend
{
auto copy_home_radio(const HomeRadio& source) -> Box<HomeRadio> {
    auto result = Box<HomeRadio>::make();
    if (source.f_id.is_some()) {
        auto id      = Box<p_qcm::p_msg::p_id::M_RadioQueueId>::make();
        id->f_value  = (*source.f_id)->f_value;
        result->f_id = Some(rstd::move(id));
    }
    result->f_name          = source.f_name.clone();
    result->f_description   = source.f_description.clone();
    result->f_can_restart   = source.f_can_restart;
    result->f_restart_label = source.f_restart_label.clone();
    for (const auto& original : source.f_configuration.as_slice()) {
        auto field           = Box<p_qcm::p_msg::p_model::M_QueueConfigField>::make();
        field->f_key         = original->f_key.clone();
        field->f_name        = original->f_name.clone();
        field->f_description = original->f_description.clone();
        field->f_value       = original->f_value.clone();
        for (const auto& option : original->f_options.as_slice()) {
            auto copy           = Box<p_qcm::p_msg::p_model::M_QueueConfigOption>::make();
            copy->f_value       = option->f_value.clone();
            copy->f_name        = option->f_name.clone();
            copy->f_description = option->f_description.clone();
            field->f_options.push(rstd::move(copy));
        }
        result->f_configuration.push(rstd::move(field));
    }
    return result;
}
class HomeTarget {
    RSTD_ENUM(HomeTarget, (Song, (qcm::backend::Song model;)),
              (Album, (qcm::backend::Album model;)), (Artist, (qcm::backend::Artist model;)),
              (Mix, (qcm::backend::Mix model;)), (Radio, (ref<HomeRadio> model;)))
};
struct HomeResolved {
    i64        id;
    ItemType   type;
    HomeTarget target;

    auto name() const -> const String& {
        RSTD_MATCH(target) {
            RSTD_CASE(Song, model) { return model.name; }
            RSTD_CASE(Album, model) { return model.name; }
            RSTD_CASE(Artist, model) { return model.name; }
            RSTD_CASE(Mix, model) { return model.name; }
            RSTD_CASE(Radio, model) { return model->f_name; }
        }
    }
    auto wire_target() const -> HomeCard::C_o_target {
        RSTD_MATCH(target) {
            RSTD_CASE(Song, model) {
                return HomeCard::C_o_target::make_song(
                    rstd::into<Box<p_qcm::p_msg::p_model::M_Song>>(
                        ref<qcm::backend::Song>::from_raw_parts(rstd::addressof(model))));
            }
            RSTD_CASE(Album, model) {
                return HomeCard::C_o_target::make_album(
                    rstd::into<Box<p_qcm::p_msg::p_model::M_Album>>(
                        ref<qcm::backend::Album>::from_raw_parts(rstd::addressof(model))));
            }
            RSTD_CASE(Artist, model) {
                return HomeCard::C_o_target::make_artist(
                    rstd::into<Box<p_qcm::p_msg::p_model::M_Artist>>(
                        ref<qcm::backend::Artist>::from_raw_parts(rstd::addressof(model))));
            }
            RSTD_CASE(Mix, model) {
                return HomeCard::C_o_target::make_mix(rstd::into<Box<p_qcm::p_msg::p_model::M_Mix>>(
                    ref<qcm::backend::Mix>::from_raw_parts(rstd::addressof(model))));
            }
            RSTD_CASE(Radio, model) {
                return HomeCard::C_o_target::make_radio(copy_home_radio(*model));
            }
        }
    }
};
// Box keeps target borrows valid when resolving a cover grows the map.
using HomeTargets = HashMap<i64, Option<Box<HomeResolved>>>;

auto read_home_target(sqlito::Transaction& tx, const Item& item, const Vec<Box<HomeRadio>>& radios)
    -> ProcessResult<Option<Box<HomeResolved>>> {
    if (item.type == ItemType::Song) {
        auto model = home_song(tx, item.id);
        if (model.is_err()) return Err(ProcessError::db(rstd::move(model).unwrap_err()));
        if (model->is_none()) return Ok<Option<Box<HomeResolved>>>(None());
        return Ok(Some(Box<HomeResolved>::make(
            HomeResolved { item.id, item.type, HomeTarget::Song(rstd::move(**model)) })));
    } else if (item.type == ItemType::Album) {
        auto model = home_album(tx, item.id);
        if (model.is_err()) return Err(ProcessError::db(rstd::move(model).unwrap_err()));
        if (model->is_none()) return Ok<Option<Box<HomeResolved>>>(None());
        return Ok(Some(Box<HomeResolved>::make(
            HomeResolved { item.id, item.type, HomeTarget::Album(rstd::move(**model)) })));
    } else if (item.type == ItemType::Artist) {
        auto model = home_artist(tx, item.id);
        if (model.is_err()) return Err(ProcessError::db(rstd::move(model).unwrap_err()));
        if (model->is_none()) return Ok<Option<Box<HomeResolved>>>(None());
        return Ok(Some(Box<HomeResolved>::make(
            HomeResolved { item.id, item.type, HomeTarget::Artist(rstd::move(**model)) })));
    } else if (item.type == ItemType::Mix) {
        auto model = home_local_mix(tx, item.id);
        if (model.is_err()) return Err(rstd::into<ProcessError>(rstd::move(model).unwrap_err()));
        if (model->is_none()) return Ok<Option<Box<HomeResolved>>>(None());
        return Ok(Some(Box<HomeResolved>::make(
            HomeResolved { (**model).id, item.type, HomeTarget::Mix(rstd::move(**model)) })));
    } else if (item.type == ItemType::Radio) {
        for (const auto& radio : radios.as_slice()) {
            if (radio->f_id.is_some() && (*radio->f_id)->f_value == item.id) {
                return Ok(Some(Box<HomeResolved>::make(
                    HomeResolved { item.id, item.type, HomeTarget::Radio(radio.as_ref()) })));
            }
        }
        return Ok<Option<Box<HomeResolved>>>(None());
    } else {
        return Err(ProcessError::internal("unsupported home target type"_str));
    }
}
auto resolve_home_target(sqlito::Transaction& tx, i64 provider, const ItemRef& reference,
                         const Vec<Box<HomeRadio>>& radios, HomeTargets& targets)
    -> ProcessResult<Option<ref<HomeResolved>>> {
    auto found = resolve_optional(tx, provider, reference, slice<ItemType> {});
    if (found.is_err()) return Err(rstd::into<ProcessError>(rstd::move(found).unwrap_err()));
    if (found->is_none()) return Ok<Option<ref<HomeResolved>>>(None());
    const auto& item   = **found;
    auto        cached = targets.get(item.id);
    if (cached.is_none()) {
        auto model = rstd_try(read_home_target(tx, item, radios));
        (void)targets.insert(item.id, rstd::move(model));
        cached = targets.get(item.id);
    }
    const auto& result = **cached;
    if (result.is_none()) return Ok<Option<ref<HomeResolved>>>(None());
    return Ok(Some((*result).as_ref()));
}
} // namespace qcm::backend

export namespace qcm::backend
{
auto home_action_supported(HomeCardAction action, ItemType type) -> bool {
    switch (action) {
    case HomeCardAction::OpenItem:
        return type == ItemType::Album || type == ItemType::Artist || type == ItemType::Mix;
    case HomeCardAction::PlayItem:
        return type == ItemType::Album || type == ItemType::Song || type == ItemType::Mix;
    case HomeCardAction::ActivateRadio: return type == ItemType::Radio;
    }
    return false;
}
auto home_action_wire(HomeCardAction action) -> i32 {
    switch (action) {
    case HomeCardAction::OpenItem: return i32(1);
    case HomeCardAction::PlayItem: return i32(2);
    case HomeCardAction::ActivateRadio: return i32(3);
    }
    return i32();
}
auto home_entry_valid(const HomeEntry& entry, ItemType type) -> bool {
    return home_action_supported(entry.primary, type) &&
           (entry.play.is_none() || home_action_supported(*entry.play, type)) &&
           ((entry.presentation.style != HomeCardStyle::Radio &&
             ! entry.presentation.cover.is_RadioCurrent()) ||
            type == ItemType::Radio);
}
auto build_home_cards(sqlito::Connection& db, SyncCoordinator& coordinator, i64 provider,
                      Vec<HomeEntry>& entries, Vec<Box<HomeRadio>>& radios,
                      p_qcm::p_msg::p_model::M_HomeRecommendation& output) -> ProcessResult<empty> {
    if (entries.len() > usize(100)) return Err(ProcessError::internal("too many home entries"_str));
    auto snapshot = coordinator.lock_snapshot();
    auto started  = db.begin();
    if (started.is_err()) return Err(ProcessError::db(rstd::move(started).unwrap_err()));
    auto        tx = rstd::move(started).unwrap();
    Vec<String> keys;
    Vec<i64>    placed;
    HomeTargets targets;
    for (const auto& entry : entries.as_slice()) {
        if (entry.key.is_empty() || entry.key.len() > usize(256))
            return Err(ProcessError::internal("invalid home entry key"_str));
        for (const auto& key : keys.as_slice())
            if (key == entry.key)
                return Err(ProcessError::internal("duplicate home entry key"_str));
        keys.push(entry.key.clone());
        if (entry.target.is_Key() && ! home_entry_valid(entry, entry.target.as_Key().value.type))
            return Err(ProcessError::internal("invalid home action"_str));
        auto target = resolve_home_target(tx, provider, entry.target, radios, targets);
        if (target.is_err()) return Err(rstd::move(target).unwrap_err());
        if (target->is_none()) {
            output.f_diagnostics.push(
                rstd::format("{}: missing target or metadata", entry.key.as_str()));
            continue;
        }
        const auto& resolved = ***target;
        if (! home_entry_valid(entry, resolved.type))
            return Err(ProcessError::internal("invalid home action"_str));
        i64      cover_id   = resolved.id;
        ItemType cover_type = resolved.type;
        if (entry.presentation.cover.is_Item()) {
            auto cover = resolve_home_target(
                tx, provider, entry.presentation.cover.as_Item().item, radios, targets);
            if (cover.is_err()) return Err(rstd::move(cover).unwrap_err());
            if (cover->is_none()) {
                output.f_diagnostics.push(
                    rstd::format("{}: missing cover or metadata", entry.key.as_str()));
                continue;
            }
            cover_id   = (***cover).id;
            cover_type = (***cover).type;
        }
        auto card                = Box<HomeCard>::make();
        auto owner               = Box<p_qcm::p_msg::p_id::M_ProviderId>::make();
        owner->f_value           = provider;
        card->f_provider_id      = Some(rstd::move(owner));
        card->f_entry_key        = entry.key.clone();
        const auto& presentation = entry.presentation;
        card->f_title =
            presentation.title.is_some() ? presentation.title->clone() : resolved.name().clone();
        card->f_subtitle       = presentation.subtitle.clone();
        card->f_badge          = presentation.badge.clone().unwrap_or(String::make());
        card->f_style          = presentation.style == HomeCardStyle::Radio ? i32(1) : i32();
        card->f_cover_kind     = presentation.cover.is_RadioCurrent() ? i32(1) : i32();
        auto image             = Box<p_qcm::p_msg::p_model::M_ImageReference>::make();
        image->f_type          = i32(static_cast<int>(cover_type));
        image->f_id            = cover_id;
        card->f_cover          = Some(rstd::move(image));
        card->f_primary_action = home_action_wire(entry.primary);
        card->f_play_action    = entry.play.is_some() ? home_action_wire(*entry.play) : i32();
        card->o_target         = Some(resolved.wire_target());
        if (resolved.type == ItemType::Radio) placed.push(i64(resolved.id));
        output.f_for_you.push(rstd::move(card));
    }
    rstd_try(tx.commit());
    for (const auto& radio : radios.as_slice()) {
        bool used = false;
        for (auto id : placed.as_slice())
            if (radio->f_id.is_some() && (*radio->f_id)->f_value == id) used = true;
        if (! used) output.f_radios.push(copy_home_radio(*radio));
    }
    return Ok(empty {});
}
auto home_candidate_async(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                          SyncCoordinator& coordinator, i64 provider_id,
                          ProviderRequestOwnerHandle                   owner,
                          p_qcm::p_msg::p_model::M_HomeRecommendation& output)
    -> rstd::async::coro<ProcessResult<empty>> {
    auto snapshot  = runtime.snapshot(provider_id);
    output.f_epoch = snapshot.is_some() ? snapshot->stamp : u64();
    if (snapshot.is_none())
        co_return Err(ProcessError::NoSuchProvider(rstd::format("{}", provider_id)));
    auto instance = snapshot->instance.clone();
    auto revision = rstd_co_try(
        co_await database_run(db.clone(),
                              [coordinator = coordinator.clone(), provider_id](
                                  sqlito::Connection&) mutable -> Result<u64, sqlito::Error> {
                                  return Ok(coordinator.write_revision(provider_id));
                              }));
    auto valid = [call = snapshot->clone()] {
        return call.is_current();
    };
    if (! valid()) co_return Err(ProcessError::NotAuth());
    auto context = snapshot->context(db.clone(), coordinator.clone());
    if (owner) {
        owner.get()->provider_id = provider_id;
        owner.get()->instance    = instance;
    }
    auto loaded = co_await instance.get()->for_you_items_async(rstd::move(context));
    if (owner) owner.get()->instance.reset();
    if (! valid()) co_return Err(ProcessError::NotAuth());
    bool fallback = false;
    if (loaded.is_err()) {
        auto error = rstd::move(loaded).unwrap_err();
        if (! error.root().is_NotImplemented())
            co_return Err(ProcessError::from_provider(rstd::move(error)));
        fallback = true;
    }
    Vec<HomeEntry> entries;
    if (! fallback)
        entries = rstd::move(*loaded);
    else {
        auto ids = rstd_co_try(
            co_await database_run(db.clone(), [provider_id](sqlito::Connection& connection) {
                return home_daily_ids(connection, provider_id);
            }));
        for (auto id : ids.as_slice()) {
            HomeEntry entry;
            entry.key    = rstd::format("daily/{}", id);
            entry.target = ItemRef::Id(id);
            entry.play   = Some(HomeCardAction::PlayItem);
            entries.push(rstd::move(entry));
        }
    }
    if (! valid()) co_return Err(ProcessError::NotAuth());
    Vec<i64> providers;
    providers.push(i64(provider_id));
    auto radios = rstd_co_try(co_await load_radio_queues_async(
        runtime, db.clone(), coordinator, rstd::move(providers), owner));
    if (! valid())
        co_return Err(
            ProcessError::internal("provider changed while loading home recommendations"_str));
    auto built = rstd_co_try(co_await database_run(
        db.clone(),
        [coordinator = coordinator.clone(),
         provider_id,
         revision,
         entries = rstd::move(entries),
         radios  = rstd::move(radios)](
            sqlito::Connection& connection) mutable -> ProcessResult<HomeRecommendation> {
            if (revision != coordinator.write_revision(provider_id))
                return Err(ProcessError::internal(
                    "provider changed while loading home recommendations"_str));
            HomeRecommendation result;
            rstd_try(
                build_home_cards(connection, coordinator, provider_id, entries, radios, result));
            if (revision != coordinator.write_revision(provider_id))
                return Err(ProcessError::internal(
                    "provider changed while loading home recommendations"_str));
            return Ok(rstd::move(result));
        }));
    if (! valid())
        co_return Err(
            ProcessError::internal("provider changed while loading home recommendations"_str));
    output.f_for_you     = rstd::move(built.f_for_you);
    output.f_radios      = rstd::move(built.f_radios);
    output.f_diagnostics = rstd::move(built.f_diagnostics);
    co_return Ok(empty {});
}
auto get_home_recommendations_async(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                                    SyncCoordinator& coordinator, i32 request_id,
                                    const p_qcm::p_msg::M_GetHomeRecommendationsReq& entry,
                                    ProviderRequestOwnerHandle                       owner = {})
    -> rstd::async::coro<ProcessResult<p_qcm::p_msg::M_QcmMessage>> {
    auto     rsp = Box<p_qcm::p_msg::M_GetHomeRecommendationsRsp>::make();
    Vec<i64> seen;
    auto     providers = rstd_co_try(checked_ids(entry.f_provider_ids, "provider_ids"_str));
    for (auto id : providers) {
        bool duplicate = false;
        for (auto previous : seen.as_slice())
            if (previous == id) duplicate = true;
        if (duplicate) continue;
        seen.push(i64(id));
        auto candidate           = Box<p_qcm::p_msg::p_model::M_HomeRecommendation>::make();
        auto provider            = Box<p_qcm::p_msg::p_id::M_ProviderId>::make();
        provider->f_value        = id;
        candidate->f_provider_id = Some(rstd::move(provider));
        auto error =
            co_await home_candidate_async(runtime, db.clone(), coordinator, id, owner, *candidate);
        if (error.is_err()) {
            candidate->f_for_you     = {};
            candidate->f_radios      = {};
            candidate->f_diagnostics = {};
            candidate->f_error       = rstd::format("{}", rstd::move(error).unwrap_err());
        }
        rsp->f_providers.push(rstd::move(candidate));
    }
    co_return Ok(qcm_message(request_id, rstd::move(rsp)));
}
} // namespace qcm::backend

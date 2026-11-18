module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
module qcm.backend;
import rstd;
import sqlito;
import :entity;
import :event;
import :playback;
import :protocol;
import :sync_events;
import :task;
import lihttpto;
import :api.comments;
import :api.home;
import :api.library;
import :api.mix;
import :api.network;
import :api.provider;
import :api.provider_status;
import :api.query;
import :api.radio;
import :api.recommendations;
import :api.storage;
import :api.request_input;
import :db;
import :db.query.media;
import :db.mix.query;
import :db.mix.refresh;
import :error;
import :error_response;
import :identity;
import :model.library;
import :model.mix;
import :pagination;
import :plugin;
import :provider;
import :resource;
import :sync_state;
import :service;
template<class T>
using AsyncMutex = rstd::async::Mutex<T>;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::tuple;
using rstd::collections::HashMap;
using rstd::path::PathBuf;
using rstd::sync::Arc;
using rstd::sync::Weak;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::async::coro;
using rstd::async::RuntimeHandle;

namespace qcm::backend
{
class QcmService::RequestDispatcher {
    Arc<QcmService> service_;

public:
    explicit RequestDispatcher(Arc<QcmService> service): service_(rstd::move(service)) {}
    auto storage_info_async(i32 request_id) -> coro<Result<QcmMessage, ProcessError>> {
        resource_cache::Usage usage;
        if (service_->resource_store_.is_some()) {
            usage = rstd_co_try(co_await (*service_->resource_store_)->usage());
        }
        i64 cache_size {};
        if (service_->cache_db_.is_some())
            cache_size = rstd_co_try(
                co_await database_run(service_->cache_db_->clone(),
                                      [](sqlito::Connection& db) -> Result<i64, sqlito::Error> {
                                          return Ok(sqlite_disk_bytes(db));
                                      }));
        auto main_size = rstd_co_try(co_await database_run(
            service_->db_.clone(), [](sqlito::Connection& db) -> Result<i64, sqlito::Error> {
                return Ok(sqlite_disk_bytes(db));
            }));
        auto total     = u64((main_size + cache_size).to_primitive());
        total          = usage.metadata > u64::MAX - total ? u64::MAX : total + usage.metadata;
        co_return Ok(storage_info_response(i64(total.to_primitive()),
                                           i64(usage.audio.to_primitive()),
                                           i64(usage.image.to_primitive()),
                                           request_id));
    }
    auto authenticate_temporary(const QcmMessage& request)
        -> coro<Result<QcmMessage, ProcessError>> {
        co_return co_await auth_provider_async(
            service_->tmp_providers_, request.f_id, *request.o_payload->as_auth_provider_req());
    }
    auto temporary_qr(const QcmMessage& request) -> coro<Result<QcmMessage, ProcessError>> {
        co_return co_await qr_auth_url_async(
            service_->tmp_providers_, request.f_id, *request.o_payload->as_qr_auth_url_req());
    }
    auto mutate_provider(const QcmMessage& request, ProviderMutationWrite& write)
        -> coro<ProcessResult<ProviderMutationOutcome>> {
        RSTD_MATCH(*request.o_payload) {
            RSTD_CASE_PAYLOAD(add_provider_req, entry) {
                service_->coordinator_.bump();
                co_return co_await add_provider_async(service_->tmp_providers_,
                                                      service_->providers_,
                                                      service_->db_.clone(),
                                                      *entry,
                                                      write);
            }
            RSTD_CASE_PAYLOAD(update_provider_req, entry) {
                auto update = rstd_co_try(provider_update_request(*entry));
                co_return co_await update_provider_async(service_->tmp_providers_,
                                                         *service_->provider_runtime_.get(),
                                                         service_->coordinator_,
                                                         service_->providers_,
                                                         service_->db_.clone(),
                                                         rstd::move(update),
                                                         write);
            }
            RSTD_CASE_PAYLOAD(replace_provider_req, entry) {
                co_return co_await replace_provider_async(service_->tmp_providers_,
                                                          *service_->provider_runtime_.get(),
                                                          service_->coordinator_,
                                                          service_->providers_,
                                                          service_->db_.clone(),
                                                          *entry,
                                                          write);
            }
            RSTD_CASE_PAYLOAD(delete_provider_req, entry) {
                co_return co_await delete_provider_async(
                    service_->db_.clone(), service_->coordinator_, *entry, write);
            }
            RSTD_CASE_DEFAULT {
                co_return Err(ProcessError::UnsupportedMessageType(request.f_type));
            }
        }
    }
    auto execute_provider_action(Arc<QcmService> service, Arc<QcmMessage> input,
                                 ProviderAction action, ProviderCancelHandle cancellable)
        -> coro<ProcessResult<QcmMessage>> {
        const auto&               request = *input;
        ProcessResult<QcmMessage> response =
            Err(ProcessError::UnsupportedMessageType(request.f_type));
        switch (action.tag) {
        case QcmPayload::Tag::auth_provider_req:
            response = co_await authenticate_temporary(request);
            break;
        case QcmPayload::Tag::qr_auth_url_req: response = co_await temporary_qr(request); break;
        case QcmPayload::Tag::delete_tmp_provider_req:
            response = delete_tmp_provider(service_->tmp_providers_,
                                           request.f_id,
                                           *request.o_payload->as_delete_tmp_provider_req());
            break;
        default: {
            ProviderMutationWrite write(rstd::move(cancellable));
            auto                  outcome = co_await mutate_provider(request, write);
            if (outcome.is_err()) {
                if (action.persisted()) co_await service_->broadcast_status();
                co_return Err(rstd::move(outcome).unwrap_err());
            }
            if (outcome->is_Committed()) {
                rstd_co_try(co_await service_->complete_provider_mutation(
                    service.clone(), outcome->as_Committed().mutation));
            } else if (action.persisted()) {
                co_await service_->broadcast_status();
            }
            response = Ok(provider_mutation_response(request.f_id, rstd::move(*outcome)));
            break;
        }
        }
        co_return co_await service_->ensure_entities(rstd_co_try(rstd::move(response)),
                                                     Some(input.clone()));
    }
    static auto run_item_sync_task(Arc<QcmService> service, Item item, ProviderHandle instance,
                                   ResponseSink sink, i32 request_id) -> coro<empty> {
        auto provider_id = item.provider_id;
        auto publisher   = service->sync_publisher(provider_id, instance);
        auto result      = co_await service->sync_provider_async(
            provider_id, Some(rstd::move(item)), rstd::move(instance));
        if (publisher.is_some()) (void)publisher->refresh_providers();
        Result<QcmMessage, ProcessError> response =
            result.is_ok() ? co_await service->ensure_entities(ok_response(request_id))
                           : Result<QcmMessage, ProcessError>(Err(rstd::move(result).unwrap_err()));
        (void)co_await sink.respond(request_id, rstd::move(response));
        co_return empty {};
    }
    auto start_item_sync_task(Arc<QcmService> service, i64 item_id, i32 request_id,
                              ResponseSink sink) -> coro<Result<empty, ProcessError>> {
        auto item = rstd_co_try(
            co_await database_run(service_->db_.clone(),
                                  [item_id](sqlito::Connection& db) -> ProcessResult<Option<Item>> {
                                      return Ok(rstd_try(read_item_by_id(db, item_id)));
                                  }));
        if (item.is_none()) co_return Err(ProcessError::NotFound());
        auto provider_id = item->provider_id;
        auto instance    = service_->provider_runtime_.get()->acquire(provider_id);
        if (! instance)
            co_return Err(ProcessError::NoSuchProvider(rstd::format("{}", provider_id)));
        rstd_co_try(service_->tasks_.get()->start_item_sync(
            provider_id,
            instance.clone(),
            request_id,
            rstd::move(sink),
            [service  = service.clone(),
             item     = rstd::move(*item),
             instance = instance.clone(),
             request_id](ResponseSink response) mutable {
                return run_item_sync_task(service.clone(),
                                          rstd::move(item),
                                          instance.clone(),
                                          rstd::move(response),
                                          request_id);
            }));
        co_return Ok(empty {});
    }
    static auto run_provider_request_task(Arc<QcmService> service, Arc<QcmMessage> input,
                                          ProviderRequestOwnerHandle owner, ResponseSink sink)
        -> coro<empty> {
        RequestDispatcher dispatcher(service.clone());
        auto              response =
            co_await dispatcher.execute_provider_request_async(input.clone(), rstd::move(owner));
        (void)co_await sink.respond(input->f_id, rstd::move(response));
        co_return empty {};
    }
    auto start_provider_request_task(Arc<QcmService> service, Arc<QcmMessage> input,
                                     ResponseSink sink) -> Result<empty, ProcessError> {
        auto request_id = input->f_id;
        rstd_try(service_->tasks_.get()->start_provider_request(
            request_id,
            rstd::move(sink),
            [service = service.clone(), input = rstd::move(input)](ProviderRequestOwnerHandle owner,
                                                                   ResponseSink response) mutable {
                return run_provider_request_task(
                    service.clone(), rstd::move(input), rstd::move(owner), rstd::move(response));
            }));
        return Ok(empty {});
    }
    auto start_provider_change(Arc<QcmService> service, RequestInput request, ResponseSink sink)
        -> bool {
        auto input      = request.message();
        auto action     = rstd::move(request).into_action();
        auto request_id = input->f_id;
        return service_->tasks_.get()
            ->start_provider_change(
                request_id,
                rstd::move(sink),
                [service = service.clone(), input = rstd::move(input), action = rstd::move(action)](
                    ProviderCancelHandle cancellable, ResponseSink response) mutable {
                    return run_provider_change(service.clone(),
                                               rstd::move(input),
                                               rstd::move(response),
                                               rstd::move(cancellable),
                                               rstd::move(action));
                })
            .is_ok();
    }
    auto execute_provider_request_async(Arc<QcmMessage> input, ProviderRequestOwnerHandle owner)
        -> coro<Result<QcmMessage, ProcessError>> {
        const auto& request = *input;
        if (request.o_payload.is_none())
            co_return Err(ProcessError::UnsupportedMessageType(request.f_type));
        RSTD_MATCH(*request.o_payload) {
            RSTD_CASE_PAYLOAD(get_home_recommendations_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(
                        co_await get_home_recommendations_async(*service_->provider_runtime_.get(),
                                                                service_->db_.clone(),
                                                                service_->coordinator_,
                                                                request.f_id,
                                                                *payload,
                                                                owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_radio_queues_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await get_radio_queues_async(*service_->provider_runtime_.get(),
                                                                service_->db_.clone(),
                                                                service_->coordinator_,
                                                                request.f_id,
                                                                *payload,
                                                                owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(open_queue_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await open_queue_async(*service_->provider_runtime_.get(),
                                                          *service_->radio_sessions_,
                                                          service_->coordinator_,
                                                          service_->db_.clone(),
                                                          request.f_id,
                                                          *payload,
                                                          owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_radio_preview_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await get_radio_preview_async(*service_->provider_runtime_.get(),
                                                                 *service_->radio_sessions_,
                                                                 service_->coordinator_,
                                                                 service_->db_.clone(),
                                                                 request.f_id,
                                                                 *payload,
                                                                 owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_queue_next_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await next_queue_async(*service_->provider_runtime_.get(),
                                                          *service_->radio_sessions_,
                                                          service_->coordinator_,
                                                          service_->db_.clone(),
                                                          request.f_id,
                                                          *payload,
                                                          owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(close_queue_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await close_queue_async(*service_->provider_runtime_.get(),
                                                           *service_->radio_sessions_,
                                                           service_->coordinator_,
                                                           service_->db_.clone(),
                                                           request.f_id,
                                                           *payload,
                                                           owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_home_blocks_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await get_home_blocks_async(*service_->provider_runtime_.get(),
                                                               service_->db_.clone(),
                                                               service_->coordinator_,
                                                               request.f_id,
                                                               *payload,
                                                               owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_home_block_items_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(
                        co_await get_home_block_items_async(*service_->provider_runtime_.get(),
                                                            service_->db_.clone(),
                                                            service_->coordinator_,
                                                            request.f_id,
                                                            *payload,
                                                            owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_comments_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await comments_async(*service_->provider_runtime_.get(),
                                                        service_->db_.clone(),
                                                        service_->coordinator_,
                                                        request.f_id,
                                                        *payload,
                                                        owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_comment_replies_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await comment_replies_async(*service_->provider_runtime_.get(),
                                                               service_->db_.clone(),
                                                               service_->coordinator_,
                                                               request.f_id,
                                                               *payload,
                                                               owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(set_comment_like_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await set_comment_like_async(*service_->provider_runtime_.get(),
                                                                service_->db_.clone(),
                                                                service_->coordinator_,
                                                                request.f_id,
                                                                *payload,
                                                                owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_subtitle_req, payload) {
                const auto& entry        = *payload;
                auto        plan         = rstd_co_try(co_await prepare_subtitle(
                    *service_->provider_runtime_.get(), service_->db_.clone(), entry));
                owner.get()->provider_id = plan.target.provider_id;
                owner.get()->instance    = plan.instance.clone();
                auto response = co_await execute_subtitle(*service_->provider_runtime_.get(),
                                                          service_->db_.clone(),
                                                          request.f_id,
                                                          rstd::move(plan));
                owner.get()->instance.reset();
                co_return co_await service_->ensure_entities(rstd_co_try(rstd::move(response)),
                                                             Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(set_favorite_req, payload) {
                const auto& entry        = *payload;
                auto        plan         = rstd_co_try(co_await prepare_favorite(
                    *service_->provider_runtime_.get(), service_->db_.clone(), entry));
                owner.get()->provider_id = plan.target.provider_id;
                owner.get()->instance    = plan.instance.clone();
                auto response = co_await execute_favorite(*service_->provider_runtime_.get(),
                                                          service_->coordinator_,
                                                          service_->db_.clone(),
                                                          request.f_id,
                                                          entry,
                                                          rstd::move(plan));
                owner.get()->instance.reset();
                co_return co_await service_->ensure_entities(rstd_co_try(rstd::move(response)),
                                                             Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_mix_songs_req, payload) {
                const auto& entry = *payload;
                auto        query = rstd_co_try(prepare_mix_songs(entry));
                rstd_co_try(co_await service_->refresh_mix_async(
                    query.mix_id, entry.f_force_refresh, owner));
                auto response = rstd_co_try(co_await query_with_entities(
                    request.f_id, [request_id = request.f_id, query](auto& db) {
                        return mix_songs(db, request_id, query);
                    }));
                if (response.is_none())
                    co_return Err(ProcessError::internal("mix query produced no response"_str));
                co_return Ok(rstd::move(*response));
            }
            RSTD_CASE_PAYLOAD(get_song_ids_req, payload) {
                auto query = rstd_co_try(query_detail::prepare_song_query(*payload));
                if (query.is_Songs())
                    for (const auto& filter : query.as_Songs().query.filters.values)
                        if (filter.value.is_MixId())
                            rstd_co_try(co_await service_->refresh_mix_async(
                                filter.value.as_MixId().id, false, owner));
                auto response = rstd_co_try(co_await query_with_entities(
                    request.f_id, [request_id = request.f_id, query = rstd::move(query)](auto& db) {
                        return query_get_song_ids_prepared(db, request_id, query);
                    }));
                if (response.is_none())
                    co_return Err(ProcessError::internal("song query produced no response"_str));
                co_return Ok(rstd::move(*response));
            }
            RSTD_CASE_DEFAULT { break; }
        }
        co_return Err(ProcessError::UnsupportedMessageType(request.f_type));
    }
    auto process_request(Arc<QcmService> service, QcmMessage request,
                         const ProtocolSession& protocol, ResponseSink sink)
        -> coro<ProcessResult<Option<QcmMessage>>> {
        rstd_co_try(protocol.check(request));
        auto prepared = rstd_co_try(RequestInput::prepare(rstd::move(request)));
        auto input    = prepared.message();
        switch (prepared.kind()) {
        case RequestKind::ItemSync: {
            rstd_co_try(co_await start_item_sync_task(service.clone(),
                                                      input->o_payload->as_sync_item_req()->f_id,
                                                      input->f_id,
                                                      sink.clone()));
            co_return Ok(None<QcmMessage>());
        }
        case RequestKind::ProviderRequest: {
            rstd_co_try(start_provider_request_task(service.clone(), input.clone(), sink.clone()));
            co_return Ok(None<QcmMessage>());
        }
        case RequestKind::ProviderChange: {
            if (! start_provider_change(service.clone(), rstd::move(prepared), sink.clone()))
                co_return Err(ProcessError::internal("provider service is stopping"_str));
            co_return Ok(None<QcmMessage>());
        }
        case RequestKind::Direct: break;
        }
        auto dispatched = rstd_co_try(co_await dispatch_async(service.clone(), input.clone()));
        if (dispatched.is_none()) co_return Ok(None<QcmMessage>());
        co_return Ok(Some(rstd_co_try(
            co_await service_->ensure_entities(rstd::move(*dispatched), Some(input.clone())))));
    }
    auto dispatch_async(Arc<QcmService> service, Arc<QcmMessage> input)
        -> coro<Result<Option<QcmMessage>, ProcessError>> {
        const auto& request = *input;
        auto        type    = request.f_type;
        if (request.o_payload->is_test_req()) {
            auto rsp         = Box<p_qcm::p_msg::M_TestRsp>::make();
            rsp->f_test_data = String::make("Echo: "_str);
            rsp->f_test_data.push_str(request.o_payload->as_test_req()->f_test_data.as_str());
            if (request.o_payload->as_test_req()->f_ui_client) rsp->f_ui_accepted = Some(true);
            auto response = qcm_message(request.f_id, rstd::move(rsp));
            co_return co_await with_entities(request.f_id, rstd::move(response));
        }
        RSTD_MATCH(*request.o_payload) {
            RSTD_CASE_PAYLOAD(player_control_req, entry) {
                p_qcm::p_msg::M_PlayerControlReq command;
                command.f_action                = entry->f_action;
                command.f_position_ms           = entry->f_position_ms;
                command.f_volume                = entry->f_volume;
                command.f_fade_ms               = entry->f_fade_ms;
                command.f_source                = entry->f_source.clone();
                command.f_expected_source_epoch = entry->f_expected_source_epoch;
                rstd_co_try(co_await service_->control_player(rstd::move(command)));
                co_return Ok(Some(ok_response(request.f_id)));
            }
            RSTD_CASE_PAYLOAD(player_queue_req, entry) {
                p_qcm::p_msg::M_PlayerQueueReq command;
                command.f_action      = entry->f_action;
                command.f_song_ids    = entry->f_song_ids.clone();
                command.f_index       = entry->f_index;
                command.f_count       = entry->f_count;
                command.f_destination = entry->f_destination;
                command.f_loop_mode   = entry->f_loop_mode;
                command.f_shuffle     = entry->f_shuffle;
                command.f_play        = entry->f_play;
                command.f_name        = entry->f_name.clone();
                if (entry->f_radio_queue_id.is_some()) {
                    auto id                  = Box<p_qcm::p_msg::p_id::M_RadioQueueId>::make();
                    id->f_value              = (*entry->f_radio_queue_id)->f_value;
                    command.f_radio_queue_id = Some(rstd::move(id));
                }
                command.f_configuration     = entry->f_configuration.clone();
                command.f_expected_revision = entry->f_expected_revision;
                command.f_random_mode       = entry->f_random_mode;
                command.f_radio_reason      = entry->f_radio_reason;
                rstd_co_try(co_await service_->edit_player_queue(rstd::move(command)));
                co_return Ok(Some(ok_response(request.f_id)));
            }
            RSTD_CASE(playlog_req) {
                co_return optional_response(co_await database_run(
                    service_->db_.clone(),
                    [input       = input.clone(),
                     playing     = service_->provider_runtime_->playing_state(),
                     coordinator = service_->coordinator_.clone()](sqlito::Connection& db) mutable {
                        return playlog(
                            db, coordinator, *playing, *input, *input->o_payload->as_playlog_req());
                    }));
            }
            RSTD_CASE(get_mixs_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return mixs(db, *input, *input->o_payload->as_get_mixs_req());
                    });
            }
            RSTD_CASE(get_mix_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return mix(db, *input, *input->o_payload->as_get_mix_req());
                    });
            }
            RSTD_CASE(get_songs_by_id_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return songs_by_id(db, *input, *input->o_payload->as_get_songs_by_id_req());
                    });
            }
            RSTD_CASE(get_album_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return album(db, *input, *input->o_payload->as_get_album_req());
                    });
            }
            RSTD_CASE(get_artist_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return artist(db, *input, *input->o_payload->as_get_artist_req());
                    });
            }
            RSTD_CASE(get_albums_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return query_get_albums(
                            db, input->f_id, *input->o_payload->as_get_albums_req());
                    });
            }
            RSTD_CASE(get_artists_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return query_get_artists(
                            db, input->f_id, *input->o_payload->as_get_artists_req());
                    });
            }
            RSTD_CASE(get_album_artists_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return query_get_album_artists(
                            db, input->f_id, *input->o_payload->as_get_album_artists_req());
                    });
            }
            RSTD_CASE(get_artist_album_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return query_get_artist_album(
                            db, input->f_id, *input->o_payload->as_get_artist_album_req());
                    });
            }
            RSTD_CASE(get_remote_mixs_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return query_get_remote_mixs(
                            db, input->f_id, *input->o_payload->as_get_remote_mixs_req());
                    });
            }
            RSTD_CASE(search_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return query_search(db, input->f_id, *input->o_payload->as_search_req());
                    });
            }
            RSTD_CASE(create_mix_req) {
                co_return optional_response(co_await database_request(
                    input.clone(), [](auto& db, auto& coordinator, const QcmMessage& request) {
                        return create_mix(
                            db, coordinator, request.f_id, *request.o_payload->as_create_mix_req());
                    }));
            }
            RSTD_CASE(mix_manipulate_req) {
                co_return optional_response(co_await database_request(
                    input.clone(), [](auto& db, auto& coordinator, const QcmMessage& request) {
                        return manipulate_mix(db,
                                              coordinator,
                                              request.f_id,
                                              *request.o_payload->as_mix_manipulate_req());
                    }));
            }
            RSTD_CASE(link_mix_req) {
                co_return optional_response(co_await database_request(
                    input.clone(), [](auto& db, auto& coordinator, const QcmMessage& request) {
                        return link_mix(
                            db, coordinator, request.f_id, *request.o_payload->as_link_mix_req());
                    }));
            }
            RSTD_CASE(delete_mix_req) {
                co_return optional_response(co_await database_request(
                    input.clone(), [](auto& db, auto& coordinator, const QcmMessage& request) {
                        return delete_mix(
                            db, coordinator, request.f_id, *request.o_payload->as_delete_mix_req());
                    }));
            }
            RSTD_CASE(get_network_settings_req) {
                co_return optional_response(co_await database_request(
                    input.clone(), [](auto& db, auto&, const QcmMessage& request) {
                        return get_network_settings(db, request.f_id);
                    }));
            }
            RSTD_CASE(update_network_settings_req) {
                service_->coordinator_.bump();
                co_return optional_response(co_await database_request(
                    input.clone(), [](auto& db, auto&, const QcmMessage& request) {
                        return update_network_settings(
                            db, request.f_id, *request.o_payload->as_update_network_settings_req());
                    }));
            }
            RSTD_CASE(get_provider_metas_req) {
                co_return optional_response(get_provider_metas(service_->providers_, request.f_id));
            }
            RSTD_CASE_PAYLOAD(create_tmp_provider_req, payload) {
                co_return optional_response(create_tmp_provider(service_->tmp_providers_,
                                                                service_->providers_,
                                                                *service_->provider_runtime_.get(),
                                                                request.f_id,
                                                                *payload));
            }
            RSTD_CASE_PAYLOAD(get_provider_settings_req, payload) {
                co_return optional_response(get_provider_settings(
                    *service_->provider_runtime_.get(), request.f_id, *payload));
            }
            RSTD_CASE_PAYLOAD(get_share_link_req, payload) {
                co_return optional_response(
                    co_await get_share_link_async(*service_->provider_runtime_.get(),
                                                  service_->db_.clone(),
                                                  request.f_id,
                                                  *payload));
            }
            RSTD_CASE_PAYLOAD(sync_req, payload) {
                auto& entry      = *payload;
                auto provider_id = rstd_co_try(required_id(entry.f_provider_id, "provider_id"_str));
                auto handle = rstd_co_try(service_->start_sync_task(service.clone(), provider_id));
                auto rsp    = Box<p_qcm::p_msg::M_SyncRsp>::make();
                rsp->f_handle = handle;
                auto response = qcm_message(request.f_id, rstd::move(rsp));
                co_return co_await with_entities(request.f_id, rstd::move(response));
            }
            RSTD_CASE(get_storage_info_req) {
                co_return optional_response(co_await storage_info_async(request.f_id));
            }
            RSTD_CASE_DEFAULT { break; }
        }
        co_return Err(ProcessError::UnsupportedMessageType(type));
    }
    static auto optional_response(QcmMessage response) -> Result<Option<QcmMessage>, ProcessError> {
        return Ok(Some(rstd::move(response)));
    }
    static auto optional_response(Result<QcmMessage, ProcessError> response)
        -> Result<Option<QcmMessage>, ProcessError> {
        return Ok(Some(rstd_try(rstd::move(response))));
    }
    template<class F>
    auto database_request(Arc<QcmMessage> input, F function)
        -> coro<Result<QcmMessage, ProcessError>> {
        co_return co_await database_run(
            service_->db_.clone(),
            [input       = rstd::move(input),
             coordinator = service_->coordinator_.clone(),
             function    = rstd::move(function)](sqlito::Connection& db) mutable {
                return function(db, coordinator, *input);
            });
    }
    auto with_entities(i32, QcmMessage response) -> coro<Result<Option<QcmMessage>, ProcessError>> {
        co_return optional_response(co_await service_->ensure_entities(rstd::move(response)));
    }
    template<class F>
    auto query_with_entities(i32, F query) -> coro<Result<Option<QcmMessage>, ProcessError>> {
        return service_->entity_queries_.query(rstd::move(query));
    }

    static auto execute_with_temporary(Arc<QcmService> service, Arc<QcmMessage> input,
                                       ResponseSink sink, ProviderAction action,
                                       ProviderCancelHandle cancellable) -> coro<empty> {
        RequestDispatcher dispatcher(service.clone());
        auto              tmp =
            action.temporary.is_empty() || action.tag == QcmPayload::Tag::delete_tmp_provider_req
                ? TmpProviderHandle {}
                : service->temporary_provider(action.temporary.as_str());
        ProcessResult<QcmMessage> response =
            Err(ProcessError::UnsupportedMessageType(input->f_type));
        if (tmp) {
            auto guard = co_await tmp.get()->gate->lock();
            response   = co_await dispatcher.execute_provider_action(
                service.clone(), input.clone(), rstd::move(action), cancellable);
        } else {
            response = co_await dispatcher.execute_provider_action(
                service.clone(), input.clone(), rstd::move(action), cancellable);
        }
        (void)co_await sink.respond(input->f_id, rstd::move(response));
        co_return empty {};
    }

    static auto run_provider_change(Arc<QcmService> service, Arc<QcmMessage> input,
                                    ResponseSink sink, ProviderCancelHandle cancellable,
                                    ProviderAction action) -> coro<empty> {
        if (action.persisted()) {
            auto lock  = service->provider_change_lock(action.provider_id.unwrap_or(i64()));
            auto guard = co_await lock->lock();
            co_await execute_with_temporary(
                service.clone(), input.clone(), sink.clone(), rstd::move(action), cancellable);
        } else {
            co_await execute_with_temporary(
                service.clone(), input.clone(), sink.clone(), rstd::move(action), cancellable);
        }
        co_return empty {};
    }
};

auto QcmService::process_request(Arc<QcmService> service, QcmMessage request,
                                 const ProtocolSession& protocol, ResponseSink sink)
    -> coro<ProcessResult<Option<QcmMessage>>> {
    RequestDispatcher dispatcher(service.clone());
    co_return co_await dispatcher.process_request(
        service.clone(), rstd::move(request), protocol, rstd::move(sink));
}

} // namespace qcm::backend

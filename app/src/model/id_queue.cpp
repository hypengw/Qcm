module;
#include <algorithm>
#include "Qcm/model/id_queue.moc.h"
module qcm;
import :model.id_queue;
import :app;

namespace qcm::model
{
DynamicIdQueue::DynamicIdQueue(const msg::id::RadioQueueId& id, QObject* parent)
    : QObject(parent), m_queue_id(id) {}

void DynamicIdQueue::updateDefinition(const QString&                             name,
                                      const QList<msg::model::QueueConfigField>& fields,
                                      bool can_restart, const QString& restart_label) {
    m_name          = name;
    m_can_restart   = can_restart;
    m_restart_label = restart_label;
    m_configuration = fields;
    if (m_has_values)
        for (auto& field : m_configuration)
            if (m_values.contains(field.key())) field.setValue(m_values.value(field.key()));
    sessionChanged();
    configurationChanged();
}

void DynamicIdQueue::applyState(const msg::PlayerQueueMsg& value) {
    if (value.radioQueueId() != m_queue_id) return;
    m_name           = value.name();
    m_values         = value.configuration();
    m_has_values     = true;
    m_exhausted      = value.exhausted();
    m_description    = value.presentation().description();
    m_current_id     = ItemId {};
    const auto ids   = value.songIds();
    const int  index = int(value.currentIndex());
    if (index >= 0 && index < ids.size())
        m_current_id = ItemId(enums::ItemType::ItemSong, qint64(ids[index]));
    for (auto& field : m_configuration)
        if (m_values.contains(field.key())) field.setValue(m_values.value(field.key()));
    disconnectQueue();
    sessionChanged();
    configurationChanged();
}

void DynamicIdQueue::disconnectQueue() {
    ++m_request_serial;
    if (std::exchange(m_configuring, false)) configuringChanged();
}
auto DynamicIdQueue::queueId() const -> msg::id::RadioQueueId { return m_queue_id; }
auto DynamicIdQueue::currentId() const -> ItemId { return m_current_id; }
auto DynamicIdQueue::name() const -> QString { return m_name; }
auto DynamicIdQueue::configuration() const -> const QList<msg::model::QueueConfigField>& {
    return m_configuration;
}
auto DynamicIdQueue::configuring() const -> bool { return m_configuring; }
auto DynamicIdQueue::exhausted() const -> bool { return m_exhausted; }
auto DynamicIdQueue::canRestart() const -> bool { return m_can_restart; }
auto DynamicIdQueue::restartLabel() const -> QString { return m_restart_label; }
auto DynamicIdQueue::description() const -> QString { return m_description; }

auto DynamicIdQueue::values() const -> QHash<QString, QString> {
    if (m_has_values) return m_values;
    QHash<QString, QString> result;
    for (const auto& field : m_configuration) result.insert(field.key(), field.value());
    return result;
}

void DynamicIdQueue::request(msg::QueueOpenReasonGadget::QueueOpenReason reason,
                             const QHash<QString, QString>&              configuration) {
    if (m_configuring) return;
    msg::PlayerQueueReq request;
    request.setAction(msg::PlayerQueueActionGadget::PlayerQueueAction::PLAYER_QUEUE_ACTION_RADIO);
    request.setRadioQueueId(m_queue_id);
    request.setRadioReason(reason);
    request.setConfiguration(configuration);
    request.setName(m_name);
    request.setPlay(true);
    m_configuring     = true;
    const auto serial = ++m_request_serial;
    configuringChanged();
    auto self = QPointer<DynamicIdQueue>(this);
    App::instance()->playqueue()->sendRequest(std::move(request), [self, serial](bool) {
        if (self && self->m_request_serial == serial) self->disconnectQueue();
    });
}
void DynamicIdQueue::activate() {
    request(msg::QueueOpenReasonGadget::QueueOpenReason::QUEUE_OPEN_REASON_INITIAL, values());
}
void DynamicIdQueue::restart() {
    if (canRestart())
        request(msg::QueueOpenReasonGadget::QueueOpenReason::QUEUE_OPEN_REASON_RESTART, values());
}
void DynamicIdQueue::configure(const QString& key, const QString& value) {
    const auto field = std::ranges::find(m_configuration, key, &msg::model::QueueConfigField::key);
    if (field == m_configuration.end() || field->value() == value) return;
    if (std::ranges::none_of(field->options(), [&value](const auto& option) {
            return option.value() == value;
        }))
        return;
    auto configuration = values();
    configuration.insert(key, value);
    request(msg::QueueOpenReasonGadget::QueueOpenReason::QUEUE_OPEN_REASON_RECONFIGURE,
            configuration);
}
} // namespace qcm::model

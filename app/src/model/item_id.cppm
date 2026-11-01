module;
#include "Qcm/message/item_id.hpp"
export module qcm:model.item_id;
export import :qml.enums;
import qextra;

export namespace qcm::msg::id
{
#define QCM_EXPORT_ID(Name, Kind, QmlName) \
    using qcm::msg::id::Name##Id;          \
    using qcm::msg::id::Name##Ids;
QCM_ID_TYPES(QCM_EXPORT_ID)
#undef QCM_EXPORT_ID
} // namespace qcm::msg::id
export namespace qcm::identity
{
using qcm::identity::fromValue;
using qcm::identity::fromValues;
using qcm::identity::IdTraits;
using qcm::identity::valid;
} // namespace qcm::identity

export namespace qcm::model
{
using qcm::model::ItemId;
using qcm::model::registerIdConverters;
} // namespace qcm::model

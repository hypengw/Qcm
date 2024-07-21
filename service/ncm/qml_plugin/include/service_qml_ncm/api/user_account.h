#pragma once
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/user_account.h"
#include "qcm_interface/model/user_account.h"

namespace qcm
{

// void handle_output(const out_type& in, const auto&) {
//     auto& o = *this;
//
//     const auto& profile = in.profile.value_or(ncm::model::UserAccountProfile {});
//     convert(o.m_userId, profile.userId);
//     convert(o.m_nickname, profile.nickname);
//     convert(o.m_avatarUrl, profile.avatarUrl);
//     emit infoChanged();
// }

namespace model
{
inline void handle_output(qcm::model::UserAccount& self, const ncm::api_model::UserAccount& in) {
    const auto& profile = in.profile.value_or(ncm::model::UserAccountProfile {});
    self.set_userId(convert_from<ItemId>(profile.userId));
    self.set_nickname(convert_from<QString>(profile.nickname));
    self.set_avatarUrl(convert_from<QString>(profile.avatarUrl));
    self.infoChanged();
}
} // namespace model

using UserAccountQuerier_base = ApiQuerier<ncm::api::UserAccount, model::UserAccount>;
class UserAccountQuerier : public UserAccountQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    UserAccountQuerier(QObject* parent = nullptr): UserAccountQuerier_base(parent) {}
};

} // namespace qcm
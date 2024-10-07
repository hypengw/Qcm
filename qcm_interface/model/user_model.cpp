#include "qcm_interface/model/user_model.h"
#include "qcm_interface/model/user_account.h"
#include "qcm_interface/global.h"

namespace qcm
{

class UserModel::Private {
public:
    Private(UserModel*): active_user(nullptr) {}
    model::UserAccount* active_user;
};

UserModel::UserModel(QObject* parent)
    : meta_model::detail::QObjectListModel<model::UserAccount>(true, parent),
      d_ptr(make_up<Private>(this)) {}

UserModel::~UserModel() {}

auto UserModel::active_user() const -> model::UserAccount* {
    C_D(const UserModel);
    return d->active_user;
}

auto UserModel::find_by_url(const QUrl& url) const -> model::UserAccount* {
    C_D(const UserModel);

    if (auto it = std::find_if(begin(),
                               end(),
                               [&url](model::UserAccount* user) {
                                   return user->userId() == url;
                               });
        it != end()) {
        return *it;
    }
    return nullptr;
}

void UserModel::set_active_user(model::UserAccount* v) {
    C_D(UserModel);
    if (std::exchange(d->active_user, v) != v) {
        activeUserChanged();
    }
}

void UserModel::add_user(model::UserAccount* user) {
    if (auto it = std::find_if(begin(),
                               end(),
                               [user](auto el) {
                                   return user->userId() == el->userId();
                               });
        it != end()) {
        auto old = *it;
        if (old != user) {
            *it = user;
            if (user) user->setParent(this);
            if (old == active_user()) {
                set_active_user(*it);
            }
            old->deleteLater();
        }
    } else {
        insert(rowCount(), user);
    }
}
void UserModel::delete_user(const model::ItemId& user_id) {
    if (auto it = std::find_if(begin(),
                               end(),
                               [user_id](auto el) {
                                   return user_id == el->userId();
                               });
        it != end()) {
        (*it)->deleteLater();
        remove(std::distance(begin(), it));
    }
}

} // namespace qcm
#include "qcm_interface/model/user_model.h"
#include "qcm_interface/model/user_account.h"
#include "qcm_interface/global.h"

namespace qcm
{

class UserModel::Private {
public:
    Private(UserModel* p): check_res(new QAsyncResult(p)), active_user(nullptr) {}
    QAsyncResult*       check_res;
    model::UserAccount* active_user;
};

UserModel::UserModel(QObject* parent)
    : meta_model::detail::QObjectListModel<model::UserAccount>(true, parent),
      d_ptr(make_up<Private>(this)) {
    connect(
        check_result(),
        &QAsyncResult::statusChanged,
        check_result(),
        [this]() {
            auto res = check_result();
            if (res->status() == QAsyncResult::Status::Finished) {
                if (auto p = qobject_cast<model::UserAccount*>(res->data())) {
                    add_user(p);
                    set_active_user(p);
                } else {
                    if (auto u = active_user()) {
                        delete_user(u->userId());
                    }
                    set_active_user(nullptr);
                }
            }
        });
}

UserModel::~UserModel() {}

void UserModel::check_user(model::UserAccount* user) {
    if (user == nullptr) {
        user = active_user();
    }
    if (user != nullptr) {
        if (auto c = Global::instance()->client(user->userId().provider().toStdString())) {
            check_result()->set_status(enums::ApiStatus::Querying);
            c.api->user_check(*c.instance, user);
            return;
        }
    }
    check_result()->set_status(enums::ApiStatus::Finished);
}

auto UserModel::check_result() const -> QAsyncResult* {
    C_D(const UserModel);
    return d->check_res;
}

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
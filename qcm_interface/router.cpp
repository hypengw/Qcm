#include "qcm_interface/router.h"

#include <set>
#include <ranges>

#include "core/helper.h"
#include "core/log.h"
#include "core/qstr_helper.h"
#include "core/qvariant_helper.h"
#include "qcm_interface/model/page.h"
#include "meta_model/qgadgetlistmodel.h"

namespace qcm
{
namespace
{
struct PathProcessImpl {
    rc<QString>                 path;
    std::vector<QStringView>    components;
    mutable Router::PathProcess process;

    bool operator<(const PathProcessImpl& b) const { return path < b.path; }
    bool operator==(const PathProcessImpl& b) const { return path == b.path; }

    static auto from(QStringView path) -> PathProcessImpl {
        if (path.startsWith('/')) path = path.sliced(1);
        PathProcessImpl out;
        out.path   = make_rc<QString>(path.toString());
        auto comps = QStringView(*out.path).split('/');
        out.components.insert(out.components.end(), comps.begin(), comps.end());
        return out;
    }
};
} // namespace

class Router::Private {
public:
    using page_model_t = meta_model::QGadgetListModel<model::Page>;

    Private(Router* p): main_page_model(new page_model_t(p)) {}

    ItemIdProcess             item_process;
    std::set<PathProcessImpl> pathes;
    page_model_t*             main_page_model;
};

Router::Router(QObject* parent): QObject(parent), d_ptr(make_up<Private>(this)) {}
Router::~Router() {}

auto Router::register_itemid(const ItemIdProcess& p) -> Router& {
    C_D(Router);
    d->item_process = p;
    return *this;
}
auto Router::register_path(QStringView path, const PathProcess& p) -> Router& {
    C_D(Router);
    auto p_impl = PathProcessImpl::from(path);
    auto it     = (d->pathes).find(p_impl);
    if (it != d->pathes.end()) {
        it->process = p;
    } else {
        p_impl.process = p;
        d->pathes.insert(p_impl);
    }
    return *this;
}
auto Router::register_path(QStringView path, const QUrl& url) -> Router& {
    C_D(Router);
    auto p = [url](std::span<const QStringView>) -> QUrl {
        return url;
    };
    register_path(path, p);
    return *this;
}

auto Router::register_path(QStringView path, QStringView url) -> Router& {
    return register_path(path, QUrl(url.toString()));
}

auto Router::route(const QUrl& url) const -> std::optional<QUrl> {
    C_D(const Router);

    auto scheme = url.scheme().toLower().toStdString();
    if (scheme == "itemid"sv) {
    } else if (scheme == "qcm"sv) {
        auto p_impl = PathProcessImpl::from(url.path());
        if (auto it = std::find_if(
                d->pathes.begin(),
                d->pathes.end(),
                [&p_impl](const PathProcessImpl& p) -> bool {
                    if (p.components.size() == p_impl.components.size()) {
                        auto view = std::ranges::zip_view(p.components, p_impl.components);

                        auto ok = std::accumulate(
                            view.begin(), view.end(), true, [](bool b, const auto& el) -> bool {
                                return b && (std::get<0>(el) == std::get<1>(el) ||
                                             std::get<1>(el) == u"{}");
                            });
                        return ok;
                    }
                    return false;
                });
            it != d->pathes.end()) {
            return it->process(p_impl.components);
        }
    }
    return std::nullopt;
}

auto Router::route_url(const QUrl& url) const -> QVariant {
    auto out = QVariant();
    convert(out, route(url));
    return out;
}
auto Router::route_page(const QUrl&) const -> QObject* { return nullptr; }

auto Router::basic_page_static(enums::PluginBasicPage p) -> QStringView {
#define X(n) u"qcm:/basic/" #n
    switch (p) {
    case enums::PluginBasicPage::BPageLogin: {
        return X(login);
    }
    }
#undef X
    return u"";
}
auto Router::basic_page(enums::PluginBasicPage p) const -> QUrl {
    return QUrl(basic_page_static(p).toString());
}
auto Router::main_page_model() const -> QAbstractListModel* {
    C_D(const Router);
    return d->main_page_model;
}
void Router::add_main_page(const model::Page& p) {
    C_D(Router);
    auto m = d->main_page_model;
    m->insert(m->rowCount(), p);
}

} // namespace qcm

#include <qcm_interface/moc_router.cpp>
#pragma once

#include <QtCore/QObject>
#include <QtCore/QAbstractListModel>

#include "core/core.h"
#include "qcm_interface/export.h"
#include "qcm_interface/enum.h"
#include "qcm_interface/item_id.h"

namespace qcm
{

namespace model
{
class Page;
}

class QCM_INTERFACE_API Router : public QObject {
    Q_OBJECT

    Q_PROPERTY(QAbstractListModel* mainPageModel READ main_page_model CONSTANT FINAL)
public:
    using ItemIdProcess = std::function<std::optional<QUrl>(const model::ItemId&)>;
    using PathProcess   = std::function<std::optional<QUrl>(std::span<const QStringView>)>;

    Router(QObject* parent = nullptr);
    ~Router();

    auto register_itemid(const ItemIdProcess&) -> Router&;
    auto register_path(QStringView, const PathProcess&) -> Router&;
    auto register_path(QStringView, const QUrl&) -> Router&;
    auto register_path(QStringView, QStringView) -> Router&;

    template<typename T>
    auto register_path(enums::PluginBasicPage page, T&& p) -> Router& {
        return register_path(basic_page(page).path(), std::forward<T>(p));
    }

    auto                 route(const QUrl&) const -> std::optional<QUrl>;
    Q_INVOKABLE QVariant route_url(const QUrl&) const;
    Q_INVOKABLE QObject* route_page(const QUrl&) const;

    Q_INVOKABLE QUrl basic_page(enums::PluginBasicPage) const;
    static auto      basic_page_static(enums::PluginBasicPage) -> QStringView;

    auto main_page_model() const -> QAbstractListModel*;
    void add_main_page(const model::Page&);

private:
    class Private;
    C_DECLARE_PRIVATE(Router, d_ptr);
};

} // namespace qcm
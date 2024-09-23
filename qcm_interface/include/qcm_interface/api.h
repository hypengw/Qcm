#pragma once

#include <QObject>
#include <QQmlParserStatus>
#include <QAbstractItemModel>

#include "qcm_interface/async.inl"
#include "qcm_interface/macro.h"
#include "qcm_interface/enum.h"
#include "qcm_interface/export.h"

namespace qcm
{

namespace detail
{

template<typename M, typename A>
concept modelable =
    requires(M t, typename A::out_type out, typename A::in_type in) { t.handle_output(out, in); };
} // namespace detail

template<typename M, typename A>
concept modelable =
    detail::modelable<M, A> && (! std::derived_from<M, QAbstractItemModel> ||
                                requires(M t, qint32 offset) { t.fetchMoreReq(offset); });

class QCM_INTERFACE_API ApiQuerierBase : public QAsyncResult, public QQmlParserStatus {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(bool autoReload READ autoReload WRITE set_autoReload NOTIFY autoReloadChanged FINAL)
    Q_PROPERTY(QObject* parent READ parent CONSTANT FINAL)

public:
    ApiQuerierBase(QObject* parent = nullptr);
    virtual ~ApiQuerierBase();
    using Status = enums::ApiStatus;

    Q_INVOKABLE void query();

    bool autoReload() const;

    bool dirty() const;
    bool is_qml_parsing() const;
    void classBegin() override;
    void componentComplete() override;

    virtual void reload() = 0;
    // virtual bool can_relaod() const = 0;

public Q_SLOTS:
    void         set_autoReload(bool);
    void         reload_if_needed();
    void         mark_dirty(bool = true);
    virtual void fetch_more(qint32);

Q_SIGNALS:
    void autoReloadChanged();

protected:
    template<typename TProp, typename TIn>
    TProp prop(const TIn& in) const {
        if constexpr (ycore::is_specialization_of_v<TIn, std::optional>) {
            if (in)
                return convert_from<TProp>(in.value());
            else {
                return {};
            }
        } else {
            return convert_from<TProp>(in);
        }
    }

    template<typename TProp, typename TOut, typename TSig>
        requires std::invocable<TSig>
    void set_prop(const TProp& v, TOut& out, TSig&& sig) {
        using out_type = std::decay_t<TOut>;
        auto set       = [this, &sig](TOut& out, const auto& v) {
            if (out != v) {
                out = v;
                this->mark_dirty();
                sig();
                this->reload_if_needed();
            }
        };
        if constexpr (std::same_as<out_type, TProp>) {
            set(out, v);
        } else if constexpr (ycore::is_specialization_of_v<out_type, std::optional>) {
            set(out, convert_from<typename out_type::value_type>(v));
        } else {
            set(out, convert_from<out_type>(v));
        }
    }

private:
    class Private;
    C_DECLARE_PRIVATE(ApiQuerierBase, d_ptr);
};

} // namespace qcm
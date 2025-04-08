#pragma once

#include <QtCore/QPointer>
#include <QtCore/QThread>
#include <QtCore/QVariant>

#include <asio/post.hpp>

#include "core/core.h"
#include "core/qasio/qt_executor.h"

namespace helper
{
template<typename T>
class QWatcher {
public:
    QWatcher(): m_ptr(nullptr) {}
    QWatcher(T* t): QWatcher() {
        if (t != nullptr) {
            auto thread = t->thread();
            m_ptr       = rc<helper>(new helper(t, thread), [](helper* h) {
                if (QThread::isMainThread() && QThread::currentThread() == h->thread) {
                    auto exec = QThread::currentThread()->property("exec");
                    if (! exec.isNull() && ! exec.value<bool>()) {
                        // thread is not in exec, delete directly
                        delete h;
                        return;
                    }
                }
                h->deleteLater();
            });
            m_ptr->moveToThread(thread);
        }
    }
    // QWatcher(QPointer<T> t): m_ptr(make_rc<QPointer<T>>(t)) {}

    ~QWatcher() {}

    QWatcher(const QWatcher&)            = default;
    QWatcher& operator=(const QWatcher&) = default;
    QWatcher(QWatcher&&)                 = default;
    QWatcher& operator=(QWatcher&&)      = default;

    T* operator->() const { return get(); }
    operator bool() const { return m_ptr && m_ptr->pointer; }
    auto get() const -> T* {
        if (m_ptr) {
            return m_ptr->pointer.load();
        }
        return nullptr;
    }

    void take_owner() const {
        if (*this) {
            this->get()->setParent(this->m_ptr.get());
        }
    }

    operator T*() const { return get(); }

    auto thread() const -> QThread* {
        if (*this) {
            return m_ptr->thread();
        };
        return nullptr;
    }

private:
    struct helper : QObject {
        std::atomic<T*> pointer;
        QThread*        thread;
        helper(T* p, QThread* t): pointer(p), thread(t) {
            connect(
                p,
                &QObject::destroyed,
                this,
                [this] {
                    pointer = nullptr;
                },
                Qt::DirectConnection);
        }
    };

    rc<helper> m_ptr;
};
} // namespace helper
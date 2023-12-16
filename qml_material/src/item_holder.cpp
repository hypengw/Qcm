#include "qml_material/item_holder.h"
#include "core/log.h"
#include <QtQuick/QQuickItem>

using namespace qml_material;

ItemHolder::ItemHolder(QObject* parent): QObject(parent), m_item(nullptr), m_visible(true) {
    connect(this, &ItemHolder::itemChanged, this, &ItemHolder::refreshParent, Qt::DirectConnection);
}
ItemHolder::~ItemHolder() {}

QObject* ItemHolder::item() const { return m_item; }
bool     ItemHolder::visible() const { return m_visible; }

void ItemHolder::setItem(QObject* item) {
    if (auto old = std::exchange(m_item, item); old != item) {
        _assert_(parent());
        if (old) {
            old->setParent(nullptr);
            if (auto quick_old = qobject_cast<QQuickItem*>(old)) {
                quick_old->setParentItem(nullptr);
            }
        }
        Q_EMIT itemChanged();
    }
}

void ItemHolder::setVisible(bool v) {
    if (std::exchange(m_visible, v) != v) {
        if (auto quick_item = qobject_cast<QQuickItem*>(m_item)) {
            quick_item->setVisible(visible());
        }
        Q_EMIT visibleChanged();
    }
}

void ItemHolder::refreshParent() {
    if (m_item) {
        auto quick_item   = qobject_cast<QQuickItem*>(m_item);
        auto quick_parent = qobject_cast<QQuickItem*>(parent());
        if (quick_item && quick_parent) {
            quick_item->setParentItem(quick_parent);
            quick_item->setVisible(visible());
        } else {
            m_item->setParent(parent());
        }
    }
}
#pragma once

#include <QtQml/QQmlEngine>
#include <QPointer>

namespace qml_material
{
class ItemHolder : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QObject* item READ item WRITE setItem NOTIFY itemChanged)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
public:
    ItemHolder(QObject* parent = nullptr);
    ~ItemHolder();

    QObject* item() const;
    bool visible() const;

public Q_SLOTS:
    void setItem(QObject*);
    void setVisible(bool);
    void refreshParent();

Q_SIGNALS:
    void itemChanged();
    void visibleChanged();

private:
    QPointer<QObject> m_item;
    bool m_visible;
};

} // namespace qml_material
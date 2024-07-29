#pragma once

#include <QQmlEngine>

#include <QClipboard>
#include <QString>

namespace qcm
{
class Clipboard : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY changed FINAL)
    Q_PROPERTY(QClipboard::Mode mode READ mode WRITE setMode NOTIFY modeChanged FINAL)

public:
    Clipboard(QObject* parent = nullptr);
    QString text() const;
    void    setText(const QString&);

    QClipboard::Mode mode() const;
    void             setMode(QClipboard::Mode);

    Q_INVOKABLE void clear();
Q_SIGNALS:
    void changed();
    void modeChanged();

private:
    QClipboard::Mode m_mode;
};
} // namespace qcm

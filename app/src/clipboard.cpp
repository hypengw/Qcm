#include "Qcm/clipboard.h"
#include <QGuiApplication>

namespace qcm
{
Clipboard::Clipboard(QObject* parent): QObject(parent), m_mode(QClipboard::Mode::Clipboard) {}

QString Clipboard::text() const { return QGuiApplication::clipboard()->text(m_mode); }
void    Clipboard::setText(const QString& data) {
    QGuiApplication::clipboard()->setText(data, m_mode);
}

QClipboard::Mode Clipboard::mode() const { return m_mode; }
void             Clipboard::setMode(QClipboard::Mode m) {
    if (std::exchange(m_mode, m) != m) {
        emit modeChanged();
    }
}

void Clipboard::clear() { QGuiApplication::clipboard()->clear(m_mode); }
} // namespace qcm

#include <Qcm/moc_clipboard.cpp>
module;
#include "Qcm/qml/clipboard.moc.h"
module qcm;
import :qml;

namespace qcm
{
Clipboard::Clipboard(QObject* parent): QObject(parent), m_mode(QClipboard::Mode::Clipboard) {
    connect(
        QGuiApplication::clipboard(), &QClipboard::changed, this, [this](QClipboard::Mode mode) {
            if (mode == m_mode) Q_EMIT changed();
        });
}

QString Clipboard::text() const {
    return supported() ? QGuiApplication::clipboard()->text(m_mode) : QString();
}
void Clipboard::setText(const QString& data) {
    if (supported()) QGuiApplication::clipboard()->setText(data, m_mode);
}

QClipboard::Mode Clipboard::mode() const { return m_mode; }
void             Clipboard::setMode(QClipboard::Mode m) {
    if (std::exchange(m_mode, m) != m) {
        emit modeChanged();
        emit changed();
    }
}

bool Clipboard::supported() const {
    auto clipboard = QGuiApplication::clipboard();
    if (! clipboard) return false;
    switch (m_mode) {
    case QClipboard::Clipboard: return true;
    case QClipboard::Selection: return clipboard->supportsSelection();
    case QClipboard::FindBuffer: return clipboard->supportsFindBuffer();
    default: return false;
    }
}
bool Clipboard::copyText(const QString& text) {
    if (text.trimmed().isEmpty() || ! supported()) return false;
    setText(text);
    return true;
}
void Clipboard::clear() {
    if (supported()) QGuiApplication::clipboard()->clear(m_mode);
}
} // namespace qcm

#include "Qcm/qml/clipboard.moc.cpp"

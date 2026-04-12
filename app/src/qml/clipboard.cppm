module;

#include "QExtra/macro_qt.hpp"
#ifdef Q_MOC_RUN
#include "Qcm/qml/clipboard.moc"
#endif
export module qcm:qml.clipboard;
export import qextra;

namespace qcm
{
export class Clipboard : public QObject {
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

    Q_SIGNAL void changed();
    Q_SIGNAL void modeChanged();

private:
    QClipboard::Mode m_mode;
};
} // namespace qcm
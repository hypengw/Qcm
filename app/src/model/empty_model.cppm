module;
#include "QExtra/macro_qt.hpp"

#ifdef Q_MOC_RUN
#include "Qcm/model/empty_model.moc"
#endif

export module qcm:model.empty_model;
export import :msg;

namespace qcm::model
{
export class EmptyModel : public QObject {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(qcm::model::Song song READ song CONSTANT FINAL)
public:
    EmptyModel(QObject* parent = nullptr): QObject(parent) {}
    ~EmptyModel() {}

    auto song() const -> const model::Song& { return m_song; }

private:
    model::Song m_song;
};
} // namespace qcm::model
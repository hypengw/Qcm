import Qcm.App as QA

QA.OrderChip {
    id: root
    required property QA.SortTypeModel model

    text: {
        const m = model;
        m.item(m.currentIndex).name;
    }
    asc: model.asc
    onClicked: {
        m_header_sort_menu.open();
    }
    QA.SortMenu {
        id: m_header_sort_menu
        anchorItem: root
        model: root.model
    }
}

set(LIBNAME qml_material)

set(SOURCES
    include/qml_material/util.h
    include/qml_material/theme.h
    include/qml_material/color.h
    include/qml_material/icon.h
    include/qml_material/type.h
    include/qml_material/round_item.h
    include/qml_material/helper.h
    include/qml_material/kirigami/wheelhandler.h
    src/kirigami/wheelhandler.cpp
    src/type.cpp
    src/color.cpp
    src/icon.cpp
    src/util.cpp
    src/theme.cpp
    src/helper.cpp
    src/round_item.cpp)
set(QML_FILES
    qml/Button.qml
    qml/Control.qml
    qml/ComboBox.qml
    qml/Dialog.qml
    qml/Divider.qml
    qml/Token.qml
    qml/IconLabel.qml
    qml/IconButton.qml
    qml/FAB.qml
    qml/Card.qml
    qml/CircularIndicator.qml
    qml/Menu.qml
    qml/MenuItem.qml
    qml/Image.qml
    qml/ListItem.qml
    qml/ListView.qml
    qml/Label.qml
    qml/FontMetrics.qml
    qml/Flickable.qml
    qml/GridView.qml
    qml/Icon.qml
    qml/Text.qml
    qml/Pane.qml
    qml/Page.qml
    qml/StackView.qml
    qml/SplitView.qml
    qml/Slider.qml
    qml/SearchBar.qml
    qml/ScrollBar.qml
    qml/Rail.qml
    qml/TextInput.qml
    qml/TextField.qml
    qml/TabBar.qml
    qml/TabButton.qml
    qml/impl/ListBusyFooter.qml
    qml/impl/Ripple.qml
    qml/impl/CursorDelegate.qml
    qml/impl/State.qml
    qml/impl/SliderHandle.qml
    qml/impl/RoundedElevationEffect.qml
    qml/impl/TextFieldEmbed.qml
    )
set(RESOURCES assets/MaterialIconsRound-Regular.otf
              assets/MaterialIconsOutlined-Regular.otf)

set_source_files_properties(qml/Token.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
include_directories(include)

qt_add_qml_module(
  ${LIBNAME}
  URI
  Qcm.Material
  STATIC
  RESOURCE_PREFIX
  /
  QML_FILES
  ${QML_FILES}
  SOURCES
  ${SOURCES}
  RESOURCES
  ${RESOURCES}
  NO_PLUGIN_OPTIONAL)

target_include_directories(
  ${LIBNAME}
  PUBLIC include
  PRIVATE "include/${LIBNAME}" "include/${LIBNAME}/kirigami")

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2)

target_link_libraries(
  ${LIBNAME}
  PUBLIC Qt6::Core Qt6::Quick Qt6::QuickControls2 core
  PRIVATE material_color)

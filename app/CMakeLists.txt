set(APP_ID
    io.github.hypengw.Qcm)
set(APP_NAME
    ${PROJECT_NAME})
set(APP_VERSION
    ${PROJECT_VERSION})
set(APP_SUMMARY
    ${PROJECT_DESCRIPTION})
set(APP_AUTHOR
    "hypeng")

find_package(
  Qt6 REQUIRED
  COMPONENTS QuickControls2 Sql)

set(SOURCES
    include/Qcm/app.h
    include/Qcm/info.h
    include/Qcm/playlist.h
    include/Qcm/qr_image.h
    include/Qcm/cache_sql.h
    include/Qcm/lyric.h
    include/Qcm/clipboard.h
    include/Qcm/player.h
    src/app.cpp
    src/playlist.cpp
    src/qr_image.cpp
    src/cache_sql.cpp
    src/lyric.cpp
    src/clipboard.cpp
    src/player.cpp
    src/fake_moc.cpp)

set(QCM_INTERFACE_HEADER
    include/qcm_interface/enum.h include/qcm_interface/global.h
    include/qcm_interface/model.h include/qcm_interface/model/user_account.h)

list(
  TRANSFORM QCM_INTERFACE_HEADER
  PREPEND "${CMAKE_SOURCE_DIR}/qcm_interface/")

set(QML_FILES
    qml/js/util.mjs
    qml/PlayBar.qml
    qml/Drawer.qml
    qml/Global.qml
    qml/page/LoginPage.qml
    qml/page/AboutPage.qml
    qml/page/AlbumDetailPage.qml
    qml/page/ArtistDetailPage.qml
    qml/page/CommentPage.qml
    qml/page/DescriptionPage.qml
    qml/page/DjradioDetailPage.qml
    qml/page/PlaylistCataloguePage.qml
    qml/page/PlaylistDetailPage.qml
    qml/page/PlaylistListPage.qml
    qml/page/PlaylistCreatePage.qml
    qml/page/MinePage.qml
    qml/page/RetryPage.qml
    qml/page/SettingsPage.qml
    qml/page/SearchPage.qml
    qml/page/TodayPage.qml
    qml/page/PlayingPage.qml
    qml/page/PlayQueuePage.qml
    qml/page/MainPage.qml
    qml/page/CloudPage.qml
    qml/page/FavPage.qml
    qml/page/MaterialTest.qml
    qml/delegate/CommentDelegate.qml
    qml/delegate/SongDelegate.qml
    qml/delegate/PicGridDelegate.qml
    qml/delegate/ProgramDelegate.qml
    qml/component/ArtistsPopup.qml
    qml/component/InfoRow.qml
    qml/component/SnakeView.qml
    qml/component/Mpris.qml
    qml/component/PlaylistListView.qml
    qml/component/SongTag.qml
    qml/component/RouteMsg.qml
    qml/component/PlaySlider.qml
    qml/component/ListDescription.qml
    qml/component/LyricView.qml
    qml/component/Image.qml
    qml/component/IconRowLayout.qml
    qml/component/RoundImage.qml
    qml/component/Leaflet.qml
    qml/component/MButton.qml
    qml/component/MRoundButton.qml
    qml/component/MGridView.qml
    qml/component/MItemDelegate.qml
    qml/component/PageContainer.qml
    qml/component/PagePopup.qml
    qml/component/PageStack.qml
    qml/component/VolumeButton.qml
    qml/action/AppendListAction.qml
    qml/action/PlaynextAction.qml
    qml/action/CopyAction.qml
    qml/action/CommentAction.qml
    qml/action/ColorSchemeAction.qml
    qml/action/SubAction.qml
    qml/menu/SongMenu.qml
    qml/menu/AlbumMenu.qml
    qml/menu/ArtistMenu.qml
    qml/menu/DjradioMenu.qml
    qml/menu/PlaylistMenu.qml
    qml/menu/ProgramMenu.qml)
set(RESOURCES
    assets/Qcm.svg)

set_source_files_properties(
  qml/Theme.qml
  PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(
  qml/Global.qml
  PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(
  app
  URI
  Qcm.App
  STATIC
  RESOURCE_PREFIX
  /
  VERSION
  ${APP_VERSION}
  QML_FILES
  ${QML_FILES}
  SOURCES
  ${QCM_INTERFACE_HEADER}
  ${SOURCES}
  RESOURCES
  ${RESOURCES}
  NO_PLUGIN_OPTIONAL)

target_sources(
  app
  PRIVATE $<$<CONFIG:Debug>:src/util/QmlStackTraceHelper.cpp>)

# target_compile_definitions(app PUBLIC $<$<CONFIG:Debug>:QT_QML_DEBUG> )
target_include_directories(
  app
  PUBLIC include
  PRIVATE src/ include/Qcm include/Qcm/util
          ${CMAKE_SOURCE_DIR}/qcm_interface/include/qcm_interface
          ${CMAKE_SOURCE_DIR}/qcm_interface/include/qcm_interface/model)
target_link_libraries(
  app
  PUBLIC Qt6::Core
         Qt6::Quick
         Qt6::QuickControls2
         Qt6::Sql
         Qt6::QuickPrivate
         asio_qt
         sv_ncm
         request
         fmt::fmt
         qr_code
         meta_model
         mpris
         media_cache
         ctre
         player
         platform
         qcm_interface)

qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION main.cpp)

qt_add_qml_module(
  ${PROJECT_NAME}
  URI
  main
  QML_FILES
  main.qml
  SOURCES
  RESOURCE_PREFIX
  /)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE app appplugin kdsingleapplication)

set(APP_DESKTOP
    assets/Qcm.desktop)
set(APP_METAINFO
    assets/Qcm.metainfo.xml)
set(APP_SVG
    assets/Qcm.svg)
configure_file(${APP_DESKTOP}.in ${APP_DESKTOP} @ONLY)
configure_file(${APP_METAINFO}.in ${APP_METAINFO} @ONLY)

install(
  TARGETS ${PROJECT_NAME}
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# qt_generate_deploy_qml_app_script( TARGET ${PROJECT_NAME} FILENAME_VARIABLE
# deploy_script NO_UNSUPPORTED_PLATFORM_ERROR
# DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM) install(SCRIPT
# ${deploy_script})

if(LINUX
   AND NOT ANDROID)
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${APP_DESKTOP}"
    RENAME ${APP_ID}.desktop
    DESTINATION share/applications)
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${APP_METAINFO}"
    RENAME ${APP_ID}.metainfo.xml
    DESTINATION share/metainfo)
  install(
    FILES ${APP_SVG}
    RENAME ${APP_ID}.svg
    DESTINATION share/icons/hicolor/scalable/apps)
endif()

qt_finalize_target(${PROJECT_NAME})

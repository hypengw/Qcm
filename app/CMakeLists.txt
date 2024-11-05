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
  COMPONENTS QuickControls2 Sql Widgets)

set(SOURCES
    include/Qcm/app.h
    include/Qcm/info.h
    include/Qcm/image_provider.h
    include/Qcm/play_queue.h
    include/Qcm/qr_image.h
    include/Qcm/lyric.h
    include/Qcm/clipboard.h
    include/Qcm/player.h
    include/Qcm/qml_util.h
    include/Qcm/storage_info.h
    include/Qcm/sql/item_sql.h
    include/Qcm/sql/cache_sql.h
    include/Qcm/sql/collection_sql.h
    include/Qcm/query/query.h
    include/Qcm/query/query_model.h
    include/Qcm/query/album_collection.h
    include/Qcm/query/artist_collection.h
    include/Qcm/query/djradio_collection.h
    include/Qcm/query/playlist_collection.h
    include/Qcm/query/album_detail.h
    include/Qcm/query/playlist_detail.h
    include/Qcm/query/djradio_detail.h
    include/Qcm/query/artist_detail.h
    include/Qcm/query/artist_albums.h
    include/Qcm/query/artist_songs.h
    include/Qcm/query/mix_create.h
    include/Qcm/query/mix_manipulate.h
    include/Qcm/query/mix_delete.h
    include/Qcm/query/comments.h
    src/action.cpp
    src/app.cpp
    src/image_provider.cpp
    src/play_queue.cpp
    src/qr_image.cpp
    src/lyric.cpp
    src/clipboard.cpp
    src/player.cpp
    src/fake_moc.cpp
    src/qml_util.cpp
    src/storage_info.cpp
    src/sql/item_sql.cpp
    src/sql/cache_sql.cpp
    src/sql/collection_sql.cpp
    src/sql/sql.cpp
    src/query.cpp)

set(QCM_INTERFACE_HEADER
    include/qcm_interface/enum.h
    include/qcm_interface/global.h
    include/qcm_interface/action.h
    include/qcm_interface/notifier.h
    include/qcm_interface/model.h
    include/qcm_interface/item_id.h
    include/qcm_interface/model/plugin_model.h
    include/qcm_interface/model/query_model.h
    )

list(
  TRANSFORM QCM_INTERFACE_HEADER
  PREPEND "${CMAKE_SOURCE_DIR}/qcm_interface/")

set(QML_FILES
    qml/js/util.mjs
    qml/PlayBar.qml
    qml/Drawer.qml
    qml/DrawerContent.qml
    qml/Global.qml
    qml/page/AboutPage.qml
    qml/page/AlbumDetailPage.qml
    qml/page/ArtistDetailPage.qml
    qml/page/CommentPage.qml
    qml/page/DescriptionPage.qml
    qml/page/RadioDetailPage.qml
    qml/page/LoginPage.qml
    qml/page/PlaylistDetailPage.qml
    qml/page/PlaylistCreatePage.qml
    qml/page/RetryPage.qml
    qml/page/SettingsPage.qml
    qml/page/SearchPage.qml
    qml/page/PlayingPage.qml
    qml/page/PlayQueuePage.qml
    qml/page/MainPage.qml
    qml/page/FavPage.qml
    qml/page/StatusPage.qml
    qml/delegate/CommentDelegate.qml
    qml/delegate/SongDelegate.qml
    qml/delegate/PicGridDelegate.qml
    qml/delegate/ProgramDelegate.qml
    qml/component/ArtistsPopup.qml
    qml/component/SnakeView.qml
    qml/component/Mpris.qml
    qml/component/SongTag.qml
    qml/component/RouteMsg.qml
    qml/component/PlaySlider.qml
    qml/component/ListDescription.qml
    qml/component/ListenIcon.qml
    qml/component/LyricView.qml
    qml/component/Image.qml
    qml/component/RoundImage.qml
    qml/component/Leaflet.qml
    qml/component/GridView.qml
    qml/component/PageContainer.qml
    qml/component/PagePopup.qml
    qml/component/PageStack.qml
    qml/component/VolumeButton.qml
    qml/action/AppendListAction.qml
    qml/action/PlaynextAction.qml
    qml/action/CommentAction.qml
    qml/action/CollectAction.qml
    qml/action/CopyAction.qml
    qml/action/ColorSchemeAction.qml
    qml/action/SubAction.qml
    qml/action/SettingAction.qml
    qml/dialog/MixCreateDialog.qml
    qml/menu/SongMenu.qml
    qml/menu/AlbumMenu.qml
    qml/menu/ArtistMenu.qml
    qml/menu/RadioMenu.qml
    qml/menu/MixMenu.qml
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

target_compile_definitions(
  app
  PUBLIC "$<$<OR:$<BOOL:${QCM_QML_DEBUG}>,$<CONFIG:Debug>>:QT_QML_DEBUG>"
         "$<$<NOT:$<TARGET_EXISTS:mpris>>:NODEBUS>")

target_include_directories(
  app
  PUBLIC include
  PRIVATE src/ include/Qcm include/Qcm/util include/Qcm/query
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
         request
         fmt::fmt
         crypto
         qr_code
         meta_model
         media_cache
         ctre
         player
         platform
         qcm_interface
         "$<$<TARGET_EXISTS:mpris>:mpris>")

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
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(IMPORT_PLUGINS
    "")
if(QCM_SERVICE_STATIC_BUILD)
  get_property(
    QCM_SERVICES GLOBAL
    PROPERTY QCM_SERVICES)
  get_property(
    QCM_SERVICES_URL GLOBAL
    PROPERTY QCM_SERVICES_URL)
  target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC ${QCM_SERVICES})
  foreach(
    url
    ${QCM_SERVICES_URL})
    string(APPEND IMPORT_PLUGINS "Q_IMPORT_QML_PLUGIN(${url}Plugin)\n")
  endforeach()
  string(REPLACE "." "_" IMPORT_PLUGINS "${IMPORT_PLUGINS}")
  configure_file(static.cpp.in static.cpp @ONLY)
  target_sources(
    ${PROJECT_NAME}
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/static.cpp")
endif()

if(ANDROID)
  set(dep_dir
      "${CMAKE_BINARY_DIR}/deps")
  set(dep_libs
      "${dep_dir}/libssl.so"
      "${dep_dir}/libcrypto.so"
      "${dep_dir}/libcurl.so"
      "${dep_dir}/libavcodec.so"
      "${dep_dir}/libavformat.so"
      "${dep_dir}/libavutil.so"
      "${dep_dir}/libswresample.so")
  set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES QT_ANDROID_EXTRA_LIBS "${dep_libs}"
               QT_ANDROID_PACKAGE_SOURCE_DIR
               "${CMAKE_CURRENT_SOURCE_DIR}/android"
               QT_ANDROID_PACKAGE_NAME "${APP_ID}")

elseif(LINUX)
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

set(APP_ID io.github.hypengw.Qcm)
set(APP_NAME Qcm)
set(APP_VERSION 1.0)

find_package(Qt6 REQUIRED COMPONENTS QuickControls2 Sql)

set(SOURCES
    include/Qcm/app.h
    include/Qcm/api.h
    include/Qcm/color.h
    include/Qcm/info.h
    include/Qcm/type.h
    include/Qcm/model.h
    include/Qcm/path.h
    include/Qcm/playlist.h
    include/Qcm/ncm_image.h
    include/Qcm/qr_image.h
    include/Qcm/cache_sql.h
    include/Qcm/model/album_detail.h
    include/Qcm/model/album_detail_dynamic.h
    include/Qcm/model/album_sub.h
    include/Qcm/model/album_sublist.h
    include/Qcm/model/artist.h
    include/Qcm/model/artist_albums.h
    include/Qcm/model/artist_sublist.h
    include/Qcm/model/login.h
    include/Qcm/model/playlist_catalogue.h
    include/Qcm/model/playlist_detail.h
    include/Qcm/model/playlist_detail_dynamic.h
    include/Qcm/model/playlist_list.h
    include/Qcm/model/playlist_subscribe.h
    include/Qcm/model/recommend_songs.h
    include/Qcm/model/recommend_resource.h
    include/Qcm/model/song_url.h
    include/Qcm/model/user_account.h
    include/Qcm/model/user_playlist.h
    include/Qcm/model/qrcode_login.h
    include/Qcm/model/qrcode_unikey.h
    include/Qcm/model/radio_like.h
    include/Qcm/model/song_like.h
    include/Qcm/util/flickable_scroll_handler.hpp
    src/app.cpp
    src/api.cpp
    src/color.cpp
    src/model.cpp
    src/path.cpp
    src/playlist.cpp
    src/ncm_image.cpp
    src/qr_image.cpp
    src/cache_sql.cpp
    src/util/flickable_scroll_handler.cpp)

set(QML_FILES
    main.qml
    qml/js/util.mjs
    qml/PlayBar.qml
    qml/Theme.qml
    qml/QA.qml
    qml/page/LoginPage.qml
    qml/page/AlbumDetailPage.qml
    qml/page/ArtistDetailPage.qml
    qml/page/DescriptionPage.qml
    qml/page/PlaylistCataloguePage.qml
    qml/page/PlaylistDetailPage.qml
    qml/page/PlaylistListPage.qml
    qml/page/MinePage.qml
    qml/page/SettingsPage.qml
    qml/page/TodayPage.qml
    qml/part/ApiContainer.qml
    qml/page/PlayingPage.qml
    qml/page/MainPage.qml
    qml/part/ArtistsPopup.qml
    qml/part/InfoRow.qml
    qml/part/SnakeBar.qml
    qml/part/SnakeView.qml
    qml/part/SongDelegate.qml
    qml/part/ListBusyFooter.qml
    qml/part/PicGridDelegate.qml
    qml/part/PlaylistListView.qml
    qml/part/SongTag.qml
    qml/part/RouteMsg.qml
    qml/part/PlaySlider.qml
    qml/component/IconRowLayout.qml
    qml/component/RoundImage.qml
    qml/component/Leaflet.qml
    qml/component/MButton.qml
    qml/component/MRoundButton.qml
    qml/component/MGridView.qml
    qml/component/MItemDelegate.qml
    qml/component/MListView.qml
    qml/component/MMenu.qml
    qml/component/MMenuItem.qml
    qml/component/MFlickable.qml
    qml/component/MPage.qml
    qml/component/MPane.qml
    qml/component/MPopup.qml
    qml/component/PageContainer.qml
    qml/component/PagePopup.qml
    qml/component/PageStack.qml
    )
set(RESOURCES assets/MaterialIconsRound-Regular.otf
              assets/MaterialIconsOutlined-Regular.otf)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/QA.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(
  app
  URI
  QcmApp
  STATIC
  RESOURCE_PREFIX
  /
  VERSION
  1.0
  QML_FILES
  ${QML_FILES}
  SOURCES
  ${SOURCES}
  RESOURCES
  ${RESOURCES}
  NO_PLUGIN_OPTIONAL)

target_include_directories(
  app
  PUBLIC include
  PRIVATE src/ include/Qcm include/Qcm/model include/Qcm/util)
target_link_libraries(
  app
  PUBLIC Qt6::Core
         Qt6::Quick
         Qt6::QuickControls2
         Qt6::Sql
         asio_qt
         sv_ncm
         request
         fmt::fmt
         qr_code
         material_helper
         meta_model
         mpris
         media_cache
         ctre
         appplugin)
target_compile_definitions(
  app PUBLIC APP_ID="${APP_ID}" APP_VERSION="${APP_VERSION}"
             APP_NAME="${APP_NAME}")

qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION main.cpp)
qt_import_qml_plugins(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE app)

install(
  TARGETS ${PROJECT_NAME}
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

qt_generate_deploy_qml_app_script(
  TARGET ${PROJECT_NAME} FILENAME_VARIABLE deploy_script
  NO_UNSUPPORTED_PLATFORM_ERROR DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM)
install(SCRIPT ${deploy_script})

if(UNIX
   AND NOT APPLE
   AND NOT ANDROID)
  install(
    FILES assets/Qcm.desktop
    RENAME ${APP_ID}.desktop
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
  install(
    FILES assets/Qcm.svg
    RENAME ${APP_ID}.svg
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps)
  install(
    FILES assets/Qcm.metainfo.xml
    RENAME ${APP_ID}.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/metainfo)
endif()

qt_finalize_target(${PROJECT_NAME})

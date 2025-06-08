set(APP_ID io.github.hypengw.Qcm)
set(APP_NAME ${PROJECT_NAME})
set(APP_VERSION ${PROJECT_VERSION})
set(APP_SUMMARY ${PROJECT_DESCRIPTION})
set(APP_AUTHOR "hypengw")

find_package(Qt6 REQUIRED COMPONENTS Quick ProtobufQuick ProtobufWellKnownTypes)
qt_standard_project_setup()

set(SOURCES
    src/action.cpp
    src/app.cpp
    src/store.cpp
    src/player.cpp
    src/backend.cpp
    src/notifier.cpp
    src/global.cpp
    src/qml/clipboard.cpp
    src/qml/qml_util.cpp
    src/qml/enum.cpp
    src/qml/duration.cpp
    src/model/lyric.cpp
    src/model/page_model.cpp
    src/model/play_queue.cpp
    src/model/id_queue.cpp
    src/model/router_msg.cpp
    src/model/app_info.cpp
    src/model/empty_model.cpp
    src/model/item_id.cpp
    src/model/store_item.cpp
    src/model/list_models.cpp
    src/model/sort_filter.cpp
    src/util/global_static.cpp
    src/util/path.cpp
    src/query/album_query.cpp
    src/query/artist_query.cpp
    src/query/mix_query.cpp
    src/query/provider_query.cpp
    src/query/lyric_query.cpp
    src/query/favorite_query.cpp
    src/query/search_query.cpp
    src/query/sync_query.cpp
    src/query/qr_query.cpp
    src/query/query.cpp
    src/query/storage_info.cpp
    src/query/query_api.cpp
    src/image_provider/response.cpp
    src/image_provider/http.cpp
    src/image_provider/qr.cpp
    src/status/provider.cpp
    src/status/process.cpp
    src/status/app_state.cpp)

set(MODULES src/mod.cppm)

set(QML_FILES
    qml/js/util.mjs
    qml/PlayBar.qml
    qml/Global.qml
    qml/Window.qml
    qml/page/detail/AlbumDetailPage.qml
    qml/page/detail/ArtistDetailPage.qml
    qml/page/detail/RadioDetailPage.qml
    qml/page/detail/MixDetailPage.qml
    qml/page/toplevel/WelcomePage.qml
    qml/page/toplevel/RetryPage.qml
    qml/page/toplevel/LoadingPage.qml
    qml/page/edit/ProviderEditPage.qml
    qml/page/AddProviderPage.qml
    qml/page/AboutPage.qml
    qml/page/CommentPage.qml
    qml/page/DescriptionPage.qml
    qml/page/ProviderMetaPage.qml
    qml/page/SearchPage.qml
    qml/page/PlayingPage.qml
    qml/page/PlayQueuePage.qml
    qml/page/MainPage.qml
    qml/page/LibraryPage.qml
    qml/page/StatusPage.qml
    qml/page/SyncPage.qml
    qml/delegate/CommentDelegate.qml
    qml/delegate/SongDelegate.qml
    qml/delegate/PicCardDelegate.qml
    qml/delegate/PicCardGridDelegate.qml
    qml/delegate/ProgramDelegate.qml
    qml/component/AuthEmail.qml
    qml/component/AuthQr.qml
    qml/component/AuthUsername.qml
    qml/component/OrderChip.qml
    qml/component/DebugRect.qml
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
    qml/component/SyncingLabel.qml
    qml/component/ByteSlider.qml
    qml/component/SettingRow.qml
    qml/component/SettingSection.qml
    qml/action/AppendListAction.qml
    qml/action/PlayAction.qml
    qml/action/PlaynextAction.qml
    qml/action/CommentAction.qml
    qml/action/CollectAction.qml
    qml/action/CopyAction.qml
    qml/action/ColorSchemeAction.qml
    qml/action/SubAction.qml
    qml/action/SettingAction.qml
    qml/action/GoToAlbumAction.qml
    qml/action/GoToArtistAction.qml
    qml/action/AddToMixAction.qml
    qml/action/MixCreateAction.qml
    qml/action/FavoriteAction.qml
    qml/dialog/MixCreateDialog.qml
    qml/dialog/GoToArtistDialog.qml
    qml/dialog/AddToMixDialog.qml
    qml/menu/SongMenu.qml
    qml/menu/AlbumMenu.qml
    qml/menu/ArtistMenu.qml
    qml/menu/RadioMenu.qml
    qml/menu/MixMenu.qml
    qml/menu/ProgramMenu.qml
    qml/menu/SortMenu.qml
    qml/page/setting/SettingPage.qml
    qml/page/setting/AudioSetting.qml
    qml/page/setting/MiscSetting.qml
    qml/page/setting/NetworkSetting.qml
    qml/page/setting/StorageSetting.qml
    qml/page/setting/ThemeSetting.qml
    qml/page/setting/ProviderManagePage.qml)
set(RESOURCES assets/Qcm.svg)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/Global.qml PROPERTIES QT_QML_SINGLETON_TYPE
                                                      TRUE)

set(QML_REG_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Qcm"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Qcm/qml"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Qcm/model"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Qcm/query"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Qcm/status"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qcm_interface")

set(APP_INFO_MACROS
    APP_ID="${APP_ID}" APP_VERSION="${APP_VERSION}" APP_AUTHOR="${APP_AUTHOR}"
    APP_NAME="${APP_NAME}" APP_SUMMARY="${APP_SUMMARY}")

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
  ${SOURCES}
  RESOURCES
  ${RESOURCES}
  NO_PLUGIN_OPTIONAL)

target_sources(app PRIVATE $<$<CONFIG:Debug>:src/util/QmlStackTraceHelper.cpp>)
target_sources(app PRIVATE FILE_SET CXX_MODULES FILES ${MODULES})

set_source_files_properties(
  "${CMAKE_CURRENT_BINARY_DIR}/app_qmltyperegistrations.cpp" TARGET_DIRECTORY
  app PROPERTIES INCLUDE_DIRECTORIES "${QML_REG_INCLUDE_DIRS}")

set_source_files_properties(
  src/player.cpp src/global.cpp src/app.cpp src/qml/qml_util.cpp
  src/model/app_info.cpp PROPERTIES COMPILE_DEFINITIONS "${APP_INFO_MACROS}")

set_target_properties(app PROPERTIES CXX_SCAN_FOR_MODULES ON
                                     QT_QMLCACHEGEN_ARGUMENTS "--only-bytecode")

target_compile_definitions(
  app PUBLIC "$<$<OR:$<BOOL:${QCM_QML_DEBUG}>,$<CONFIG:Debug>>:QT_QML_DEBUG>"
             "$<$<NOT:$<TARGET_EXISTS:mpris>>:NODEBUS>")

target_compile_definitions(
  app
  PUBLIC APP_ID="${APP_ID}" APP_VERSION="${APP_VERSION}"
         APP_AUTHOR="${APP_AUTHOR}" APP_NAME="${APP_NAME}"
         APP_SUMMARY="${APP_SUMMARY}")

add_library(qcm.log STATIC src/log.cpp)
target_link_libraries(qcm.log PUBLIC core)

target_include_directories(
  app
  PUBLIC include
  PRIVATE src/)
target_link_libraries(
  app
  PUBLIC Qt6::Core
         Qt6::Quick
         "$<$<CONFIG:Debug>:Qt6::QuickPrivate>"
         core.qasio
         ncrequest::ncrequest
         rstd::rstd
         crypto
         qr_code
         meta_model
         ctre::ctre
         player
         platform
         "message"
         #message_qtprotoreg
         #messageplugin
         "$<$<TARGET_EXISTS:mpris>:mpris>")

qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION src/main.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES 
	CXX_SCAN_FOR_MODULES ON 
	WIN32_EXECUTABLE FALSE)
target_compile_definitions(${PROJECT_NAME} PRIVATE "${APP_INFO_MACROS}")

target_link_libraries(
  ${PROJECT_NAME} PRIVATE app appplugin kdsingleapplication
                          "$<LINK_GROUP:RESCAN,core,qcm.log>")

set(APP_DESKTOP assets/Qcm.desktop)
set(APP_METAINFO assets/Qcm.metainfo.xml)
set(APP_SVG assets/Qcm.svg)
configure_file(${APP_DESKTOP}.in ${APP_DESKTOP} @ONLY)
configure_file(${APP_METAINFO}.in ${APP_METAINFO} @ONLY)

install(
  TARGETS ${PROJECT_NAME}
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(IMPORT_PLUGINS "")
if(QCM_SERVICE_STATIC_BUILD)
  get_property(QCM_SERVICES GLOBAL PROPERTY QCM_SERVICES)
  get_property(QCM_SERVICES_URL GLOBAL PROPERTY QCM_SERVICES_URL)
  target_link_libraries(${PROJECT_NAME} PUBLIC ${QCM_SERVICES})
  foreach(url ${QCM_SERVICES_URL})
    string(APPEND IMPORT_PLUGINS "Q_IMPORT_QML_PLUGIN(${url}Plugin)\n")
  endforeach()
  string(REPLACE "." "_" IMPORT_PLUGINS "${IMPORT_PLUGINS}")
  configure_file(static.cpp.in static.cpp @ONLY)
  target_sources(${PROJECT_NAME}
                 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/static.cpp")
endif()

if(ANDROID)
  set(dep_dir "${CMAKE_BINARY_DIR}/deps")
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

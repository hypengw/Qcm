find_package(Qt6 REQUIRED COMPONENTS Core)


set(SOURCES
    include/service_qml_ncm/api/user_account.h

    include/service_qml_ncm/model.h
    include/service_qml_ncm/model/album_detail.h
    include/service_qml_ncm/model/album_detail_dynamic.h
    include/service_qml_ncm/model/album_sub.h
    include/service_qml_ncm/model/album_sublist.h
    include/service_qml_ncm/model/artist.h
    include/service_qml_ncm/model/artist_albums.h
    include/service_qml_ncm/model/artist_sub.h
    include/service_qml_ncm/model/artist_sublist.h
    include/service_qml_ncm/model/comments.h
    include/service_qml_ncm/model/cloudsearch.h
    include/service_qml_ncm/model/djradio_detail.h
    include/service_qml_ncm/model/djradio_sub.h
    include/service_qml_ncm/model/djradio_sublist.h
    include/service_qml_ncm/model/djradio_program.h
    include/service_qml_ncm/model/login.h
    include/service_qml_ncm/model/logout.h
    include/service_qml_ncm/model/playlist_catalogue.h
    include/service_qml_ncm/model/playlist_detail.h
    include/service_qml_ncm/model/playlist_detail_dynamic.h
    include/service_qml_ncm/model/playlist_list.h
    include/service_qml_ncm/model/playlist_subscribe.h
    include/service_qml_ncm/model/playlist_create.h
    include/service_qml_ncm/model/playlist_delete.h
    include/service_qml_ncm/model/playlist_tracks.h
    include/service_qml_ncm/model/recommend_songs.h
    include/service_qml_ncm/model/recommend_resource.h
    include/service_qml_ncm/model/song_url.h
    include/service_qml_ncm/model/song_lyric.h
    include/service_qml_ncm/model/user_playlist.h
    include/service_qml_ncm/model/user_cloud.h
    include/service_qml_ncm/model/qrcode_login.h
    include/service_qml_ncm/model/qrcode_unikey.h
    include/service_qml_ncm/model/radio_like.h
    include/service_qml_ncm/model/song_like.h
    model.cpp
    api.cpp)

qt_add_qml_module(
  service_qml_ncm
  URI
  Qcm.Service.Ncm
  STATIC
  RESOURCE_PREFIX
  /
  VERSION
  ${PROJECT_VERSION}
  QML_FILES
  ${QML_FILES}
  SOURCES
  ${SOURCES}
  RESOURCES
  ${RESOURCES}
  NO_PLUGIN_OPTIONAL)

target_include_directories(
  service_qml_ncm
  PUBLIC include
  PRIVATE . include/service_qml_ncm 
  include/service_qml_ncm/model
  include/service_qml_ncm/api
  )

target_link_libraries(service_qml_ncm PUBLIC sv_ncm qcm_interface meta_model)

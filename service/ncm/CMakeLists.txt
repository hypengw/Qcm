add_library(
  sv_ncm STATIC
  include/ncm/client.h
  include/ncm/crypto.h
  include/ncm/api.h
  include/ncm/api/album_detail.h
  include/ncm/api/album_detail_dynamic.h
  include/ncm/api/album_sub.h
  include/ncm/api/album_sublist.h
  include/ncm/api/artist.h
  include/ncm/api/artist_sublist.h
  include/ncm/api/login.h
  include/ncm/api/playlist_detail.h
  include/ncm/api/playlist_detail_dynamic.h
  include/ncm/api/recommend_songs.h
  include/ncm/api/song_url.h
  include/ncm/api/user_account.h
  include/ncm/api/user_playlist.h
  include/ncm/api/qrcode_unikey.h
  client.cpp
  crypto.cpp
  dump.h
  dump.cpp
  include/ncm/type.h
  include/ncm/model.h
  model.cpp)

target_include_directories(sv_ncm PUBLIC include .)

target_link_libraries(sv_ncm PUBLIC core asio_helper crypto request json_helper
                                    error)

set_property(TARGET sv_ncm PROPERTY POSITION_INDEPENDENT_CODE ON)

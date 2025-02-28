# Qcm
Material You cloud music player.  

This repository contains the frontend application only. To use Qcm, you'll need to run it with a compatible local backend.  

The backend process is managed by this front, you can use `--backend <path>` to set the backend executable path.  

### Backend Support

A backend implementation can be found at [QcmBackend](https://github.com/hypengw/QcmBackend), which supports the following music services:

- **Jellyfin**
- **Netease Cloud Music**

### Screenshots

<table>
  <tr>
  <td><img src="https://github.com/hypengw/Qcm/blob/master/app/assets/screenshots/main.png?raw=true"></td>
  <td><img src="https://github.com/hypengw/Qcm/blob/master/app/assets/screenshots/main_compact.png?raw=true"></td>
  </tr>
  <tr>
  <td><img src="https://github.com/hypengw/Qcm/blob/master/app/assets/screenshots/main_dark.png?raw=true"></td>
  <td><img src="https://github.com/hypengw/Qcm/blob/master/app/assets/screenshots/main_compact_dark.png?raw=true"></td>
  </tr>
</table>

### Download:  
<a href='https://flathub.org/apps/details/io.github.hypengw.Qcm'><img width='240' alt='Download on Flathub' src='https://dl.flathub.org/assets/badges/flathub-badge-en.png'/></a>

### Require:  
- Qt 6.8 (quick, dbus)
- C++ 23
- Openssl 3
- FFmpeg 7
- Curl
- QcmBackend (rust)

### Build:  
```
git clone https://github.com/hypengw/Qcm.git  
git submodule update --init

cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Release  
cmake --build build

# run without install
export QML_IMPORT_PATH=$PWD/build/qml_modules
./build/app/Qcm --backend <backend executable>

# install
cmake --install build
```

### Faq:
- Desktop lyrics
  > use [waylyrics](https://github.com/waylyrics/waylyrics)

- How to debug in flatpak
  ```bash
  flatpak install io.github.hypengw.Qcm.Debug
  flatpak run --devel --command=bash io.github.hypengw.Qcm
  # 1. run directly
  [📦 io.github.hypengw.Qcm ~]$ gdb Qcm
  (gdb) run
  Enable debuginfod for this session? (y or [n]) n
  ...
  # get the stacktrace
  (gdb) bt

  # 2. or use coredump file
  coredumpctl dump <id> -o core.save
  flatpak run --devel --filesystem=host --command=bash io.github.hypengw.Qcm
  [📦 io.github.hypengw.Qcm ~]$ gdb Qcm core.save
  ...
  ```
  

### Todo:
- [ ] subsonic
- [ ] mac/win
- [ ] offline mode
- [ ] playing page colorpick
- [ ] playing page blur
- [ ] android
- [ ] sql cjk fts
- [ ] drop qsql
- [x] jellyfin
- [x] separate front/backend to processes
- [x] sql fts
- [x] private radio
- [x] sql api model
- [x] user session switch
- [x] feedback
- [x] upload
- [x] fade in/out
- [x] sidebar popup
- [x] search page
- [x] lyric
- [x] audio cache using http proxy(AndroidVideoCache)
- [x] cache limit
- [x] sql cache
- [x] mpris
- [x] api
- [x] json
- [x] http lib(libcurl)

### Credits:
#### Libraries Used
- [Qt](https://www.qt.io/)
- [ffmpeg](https://www.ffmpeg.org/)
- [curl](https://curl.se/)
- [openssl](https://www.openssl.org/)
- [asio](https://github.com/chriskohlhoff/asio)
- [cubeb](https://github.com/mozilla/cubeb)
- [PEGTL](https://github.com/taocpp/PEGTL)
- [nlohmann/json](https://github.com/nlohmann/json)
- [fmt](https://github.com/fmtlib/fmt)
- [ctre](https://github.com/hanickadot/compile-time-regular-expressions)
- [ncrequest](https://github.com/hypengw/ncrequest)

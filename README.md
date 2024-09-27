# Qcm
Material3 cloud music player.
Currently linux only.  

<p align="center" width="100%">
<img src="https://github.com/hypengw/Qcm/blob/master/app/assets/screenshots/main.png?raw=true" width=70%>
</p>


### Muisc Service
- Jellyfin(wip)
- Netease Cloud Music

### Download:  
<a href='https://flathub.org/apps/details/io.github.hypengw.Qcm'><img width='240' alt='Download on Flathub' src='https://dl.flathub.org/assets/badges/flathub-badge-en.png'/></a>

### Require:  
- Qt 6.7 (quick, dbus)
- C++ 23
- Openssl 3
- Curl
- FFmpeg 6

### Build:  
```
git clone https://github.com/hypengw/Qcm.git  
git submodule update --init

cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Release  
cmake --build build

# run without install
export QML_IMPORT_PATH=$PWD/build/qml_modules
./build/app/Qcm

# install
cmake --install build
```

### Todo:
- [ ] jellyfin
- [ ] subsonic
- [ ] sql api cache
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

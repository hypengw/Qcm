# Qcm
Material You cloud music player.  

To use Qcm, you'll need to run it with a compatible local backend.  

The backend process is managed by Qcm, you can use `--backend <path>` to set the backend executable path.  

### Backend

A backend implementation can be found at [QcmBackend](https://github.com/hypengw/QcmBackend), which supports the following music services:

- **Jellyfin**
- **Netease Cloud Music**(wip)

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

## Installation

### Flathub

<a href='https://flathub.org/apps/details/io.github.hypengw.Qcm'><img width='240' alt='Download on Flathub' src='https://dl.flathub.org/assets/badges/flathub-badge-en.png'/></a>

### Arch Linux
#### AUR

```shell
[yay/paru] -S qcm
```

#### [archlinuxcn](http://github.com/archlinuxcn/repo) repository

```shell
sudo pacman -S qcm
```

### Require:  
- Compiler: clang 19+
- OS: linux, win(libc++)

#### Separated Libraries
The following libraries were extracted and modularized during Qcm's development.  
- [QmlMaterial](https://github.com/hypengw/QmlMaterial.git)
- [MetaModel](https://github.com/hypengw/MetaModel.git)
- [ncrequest](https://github.com/hypengw/ncrequest.git)
- [rstd](https://github.com/hypengw/rstd.git)

#### Third Libraries
- [Qt](https://www.qt.io/)
- [ffmpeg](https://www.ffmpeg.org/)
- [curl](https://curl.se/)
- [openssl](https://www.openssl.org/)
- [asio](https://github.com/chriskohlhoff/asio)
- [cubeb](https://github.com/mozilla/cubeb)
- [PEGTL](https://github.com/taocpp/PEGTL)
- [ctre](https://github.com/hanickadot/compile-time-regular-expressions)

## Build
```shell
git clone --recursive https://github.com/hypengw/Qcm.git

cmake -S Qcm -B build -GNinja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# run without install
export QML_IMPORT_PATH=$PWD/build/qml_modules
./build/app/Qcm --backend <backend executable>

# install
cmake --install build
```

## FAQ
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
- [ ] mac/win
- [ ] offline mode
- [ ] playing page colorpick
- [ ] playing page blur
- [ ] android

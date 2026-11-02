# Qcm
Quick cloud music player.  

### Backend

- **Jellyfin**
- **Subsonic**
- **Netease Cloud Music**

### Screenshots

<table>
  <tr>
  <td><img src="https://github.com/hypengw/Qcm/blob/master/app/assets/screenshots/main.jpg?raw=true"></td>
  </tr>
  <tr>
  <td><img src="https://github.com/hypengw/Qcm/blob/master/app/assets/screenshots/main_dark.jpg?raw=true"></td>
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
- [ncrequest](https://github.com/hypengw/ncrequest.git)
- [rstd](https://github.com/litocpp/rstd.git)

#### Third Libraries
- [Qt](https://www.qt.io/) 6.10
- [ffmpeg](https://www.ffmpeg.org/)
- [curl](https://curl.se/)
- [openssl](https://www.openssl.org/)
- [qr_code](https://github.com/nayuki/QR-Code-generator/tree/master/cpp)

## Build

Install Lito, Rust/Cargo, CMake, Ninja, pkg-config, Protocol Buffers tools, and the
development packages for the libraries listed above. Qt must include its private
headers, Qt Protobuf, ShaderTools, WebSockets, and QML development tools.

```shell
git clone --recursive https://github.com/hypengw/Qcm.git
cd Qcm

# build the application and backend
lito install --prefix ./install

# run the installed application
./install/bin/Qcm
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

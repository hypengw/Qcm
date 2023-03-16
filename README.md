# Qcm
Qt client for netease cloud music.  
Linux only.  

<p align="center" width="100%">
<img src="https://github.com/hypengw/Qcm/blob/master/app/assets/screenshots/main.jpg?raw=true" width=70%>
</p>

### Require:  
- Qt 6.4
- C++ 20
- Openssl 3

### Build:  
```
git clone https://github.com/hypengw/Qcm.git  
git submodule update --init
mkdir build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release  
ninja
./app/QcmApp
```

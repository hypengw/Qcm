# Qcm
Qt client for netease cloud music.  
Linux only.  

Require:  
1.Qt 6.4  
2.C++ 20  

Build:  
```
git clone https://github.com/hypengw/Qcm.git  
git submodule update --init
mkdir build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release  
ninja
./app/QcmApp
```

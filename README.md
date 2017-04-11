# instant_meshes library
This is a clone of https://github.com/wjakob/instant-meshes modified to make it into a library.

Usage:

```
git clone --recursive "https://github.com/Volumental/instant-meshes" instant_meshes
cd instant_meshes
mkdir -p build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH="your/path/here" -DEIGEN3_INCLUDE_DIR="path/to/eigen" ..
make -j7
make install
```

#!/bin/bash

CONFIG=\
"--enable-runtime-cpudetect --disable-debug "\
"--disable-all --enable-avcodec --enable-avutil "\
"--enable-parser=ac3 --enable-parser=mlp "\
"--enable-decoder=ac3 --enable-decoder=eac3 --enable-decoder=truehd --enable-decoder=mlp "\
"--enable-decoder=aac --enable-decoder=dca --enable-decoder=vorbis "\
"--enable-decoder=ape --enable-decoder=alac --enable-decoder=flac" 

mkdir build
cd build

###########################################################
## 64bit shared (dll) and static lib

mkdir shared_x64
cd shared_x64

../../configure --toolchain=msvc --arch=amd64 --prefix=../../bin/x64 --enable-shared $CONFIG

make
make install
cd ..

mkdir static_x64
cd static_x64

../../configure --toolchain=msvc --arch=amd64 --prefix=../../bin/x64 --enable-static $CONFIG

make
make install
cd ..

cd ..

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
## 32bit shared (dll) and static lib

mkdir shared_x86
cd shared_x86

../../configure --toolchain=msvc --arch=i686 --prefix=../../bin/x86 --enable-shared $CONFIG

make
make install
cd ..

mkdir static_x86
cd static_x86

../../configure --toolchain=msvc --arch=i686 --prefix=../../bin/x86 --enable-static $CONFIG

make
make install
cd ..

cd ..

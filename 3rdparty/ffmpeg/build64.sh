#!/bin/sh

PATH="/mingw64/bin:$PATH"

mkdir build_x64
cd build_x64

../configure --sysroot=/mingw64/bin --cross-prefix=x86_64-w64-mingw32- --arch=x86_64 --target-os=mingw32 --build-suffix=64 --prefix=../bin/x64 \
--enable-shared --disable-static --enable-memalign-hack --enable-runtime-cpudetect --disable-debug \
--disable-doc --disable-ffmpeg --disable-ffplay --disable-ffprobe \
--disable-ffserver --disable-avdevice --disable-avformat \
--disable-swscale --disable-postproc --disable-avfilter --disable-network \
--disable-yasm \
--disable-everything \
--enable-parser=ac3 --enable-parser=mlp \
--enable-decoder=ac3 --enable-decoder=eac3 --enable-decoder=truehd --enable-decoder=mlp \
--enable-decoder=aac --enable-decoder=dca --enable-decoder=vorbis \
--enable-decoder=ape --enable-decoder=alac --enable-decoder=flac 

make

# 'make install' requires these files to exist
touch libavcodec/avcodec64.lib
touch libavutil/avutil64.lib
make install
cd ..

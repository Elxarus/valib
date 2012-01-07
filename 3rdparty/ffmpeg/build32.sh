#!/bin/sh

PATH="/mingw32/bin:$PATH"

mkdir build_x86
cd build_x86

../configure --cross-prefix=i686-w64-mingw32- --arch=i686 --target-os=mingw32 --prefix=../bin/x86 \
--enable-shared --disable-static --enable-memalign-hack --enable-runtime-cpudetect --disable-debug \
--extra-cflags='-Dstrtod=__strtod' \
--disable-doc --disable-ffmpeg --disable-ffplay --disable-ffprobe \
--disable-ffserver --disable-avdevice --disable-avformat \
--disable-swscale --disable-postproc --disable-avfilter --disable-network \
--disable-yasm \
--disable-everything \
--enable-parser=ac3 --enable-parser=mlp \
--enable-decoder=ac3 --enable-decoder=eac3 --enable-decoder=truehd --enable-decoder=mlp \
--enable-decoder=flac 

make

# 'make install' requires these files to exist
touch libavcodec/avcodec.lib
touch libavutil/avutil.lib
make install
cd ..

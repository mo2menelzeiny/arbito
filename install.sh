#!/usr/bin/env bash

echo "Installing FIX8 Runtime.."
wget https://github.com/fix8/fix8/archive/1.4.0.tar.gz
tar zvxf 1.4.0.tar.gz
cd fix8-1.4.0
./bootstrap && ./bootstrap
./configure --prefix=/usr/local --with-mpmc=tbb --enable-tbbmalloc=yes --with-thread=stdthread --enable-preencode=yes \
 --enable-rawmsgsupport=yes --enable-ssl=yes --enable-codectiming=yes --enable-fillmetadata=no --enable-f8test=no \
 --enable-doxygen=no
make
make install
cd ..

echo "Building executables.."
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
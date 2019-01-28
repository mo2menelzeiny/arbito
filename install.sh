#!/usr/bin/env bash

mkdir externals
cd externals

echo "Installing Disruptor.."
wget https://github.com/Abc-Arbitrage/Disruptor-cpp/archive/master.zip
unzip master.zip
rm master.zip
cd Disruptor-cpp-master
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DDISRUPTOR_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX:PATH=/usr/local
cmake --build . --target install -- -j 2

#Leave to root
cd ../../..

echo "Building executables.."
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target arbito -- -j 2
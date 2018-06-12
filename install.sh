#!/usr/bin/env bash

echo "Installing Installing Disruptor.."
wget https://github.com/Abc-Arbitrage/Disruptor-cpp/archive/master.zip
unzip master.zip
cd Disruptor-cpp-master
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
make install PREFIX=/usr/local

echo "Installing Installing Aeron.."
wget https://github.com/real-logic/aeron/archive/1.9.3.tar.gz
tar zvxf 1.9.3.tar.gz
cd aeron-1.9.3
mkdir -p cppbuild/Debug && cd cppbuild/Debug
cmake -DBUILD_AERON_DRIVER=ON ../..
make
make install PREFIX=/usr/local

echo "Building executables.."
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
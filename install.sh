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
cmake .. -DDISRUPTOR_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release
make -j 2 install PREFIX=/usr/local

#return to externals/
cd ../..

echo "Installing Mongo C Driver.."
wget https://github.com/mongodb/mongo-c-driver/releases/download/1.12.0/mongo-c-driver-1.12.0.tar.gz
tar xzf mongo-c-driver-1.12.0.tar.gz
rm mongo-c-driver-1.12.0.tar.gz
cd mongo-c-driver-1.12.0
mkdir cmake-build
cd cmake-build
cmake .. -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DCMAKE_BUILD_TYPE=Release
make -j 2 install PREFIX=/usr/local

#Leave to root
cd ../../..


echo "Building executables.."
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j 2
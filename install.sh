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
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j 4
make -j 4 install PREFIX=/usr/local

#return to externals/
cd ../..

echo "Installing Aeron.."
wget https://github.com/real-logic/aeron/archive/1.9.3.tar.gz
tar zvxf 1.9.3.tar.gz
rm 1.9.3.tar.gz
cd aeron-1.9.3
mkdir -p cppbuild/Debug
cd cppbuild/Debug
cmake -DBUILD_AERON_DRIVER=ON ../..
make -j 4
make -j 4 install PREFIX=/usr/local

##return to externals/
cd ../../..

echo "Installing Mongo C Driver.."
wget https://github.com/mongodb/mongo-c-driver/releases/download/1.12.0/mongo-c-driver-1.12.0.tar.gz
tar xzf mongo-c-driver-1.12.0.tar.gz
rm mongo-c-driver-1.12.0.tar.gz
cd mongo-c-driver-1.12.0
mkdir cmake-build
cd cmake-build
cmake .. -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DCMAKE_BUILD_TYPE=Release
make -j 4
make -j 4 install PREFIX=/usr/local

#Leave to root
cd ../../../..


echo "Building executables.."
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j 4
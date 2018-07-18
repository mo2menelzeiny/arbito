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

##return to externals
#cd ../../..
#
#echo "Installing SBE.."
#wget https://github.com/real-logic/simple-binary-encoding/archive/1.8.1.tar.gz
#tar zvxf simple-binary-encoding-1.8.1.tar.gz
#rm simple-binary-encoding-1.8.1.tar.gz
#cd simple-binary-encoding-1.8.1
#./gradlew
#mkdir -p cppbuild/Debug
#cd cppbuild/Debug
#cmake  ../..
#make -j 4
#make -j 4 install PREFIX=/usr/local

#Leave to root
cd ../../../..


echo "Building executables.."
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j 4
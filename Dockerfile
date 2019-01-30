FROM ubuntu:xenial

RUN apt-get -y update && apt-get install -y

RUN apt-get -y install\
 wget\
 unzip\
 gcc\
 g++\
 cmake\
 pkg-config\
 libboost-all-dev\
 libssl-dev

WORKDIR /usr/arbito/externals

RUN wget https://github.com/Abc-Arbitrage/Disruptor-cpp/archive/master.zip\
 && unzip master.zip\
 && rm master.zip\
 && cd Disruptor-cpp-master\
 && mkdir build\
 && cd build\
 && cmake .. -DCMAKE_BUILD_TYPE=Release -DDISRUPTOR_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX:PATH=/usr/local\
 && cmake --build . --target install -- -j 2

WORKDIR /usr/arbito/externals

RUN wget https://github.com/mongodb/mongo-c-driver/releases/download/1.13.1/mongo-c-driver-1.13.1.tar.gz\
 && tar xzf mongo-c-driver-1.13.1.tar.gz\
 && rm mongo-c-driver-1.13.1.tar.gz\
 && cd mongo-c-driver-1.13.1\
 && mkdir cmake-build\
 && cd cmake-build\
 && cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DCMAKE_INSTALL_PREFIX:PATH=/usr/local\
 -DENABLE_TESTS=OFF -DENABLE_EXAMPLES=OFF\
 && cmake --build . --target install -- -j 2

COPY . /usr/arbito

WORKDIR /usr/arbito/build

RUN cmake .. -DCMAKE_BUILD_TYPE=Release\
 && cmake --build . --target arbito -- -j 2
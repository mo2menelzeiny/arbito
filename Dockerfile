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
 libssl-dev\
 libmongoc-dev\
 libbson-dev

RUN mkdir externals\
 && cd externals\
 && wget https://github.com/Abc-Arbitrage/Disruptor-cpp/archive/master.zip\
 && unzip master.zip\
 && rm master.zip\
 && cd Disruptor-cpp-master\
 && mkdir build\
 && cd build\
 && cmake .. -DCMAKE_BUILD_TYPE=Release -DDISRUPTOR_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX:PATH=/usr/local\
 && cmake --build . --target install -- -j 2\
 && cd ../../..

COPY . /usr/src/arbito
WORKDIR /usr/src/arbito

RUN mkdir build\
 && cd build\
 && cmake .. -DCMAKE_BUILD_TYPE=Release\
 && cmake --build . --target arbito -- -j 2
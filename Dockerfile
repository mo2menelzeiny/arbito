FROM ubuntu:xenial

RUN apt-get -y update && apt-get install -y

RUN apt-get -y install\
 wget\
 unzip\
 gcc\
 g++\
 cmake\
 libboost-all-dev\
 libssl-dev\
 libmongoc-dev\
 libbson-dev

COPY . /usr/src/arbito

WORKDIR /usr/src/arbito

RUN mkdir externals\
 && cd externals\
 && wget https://github.com/Abc-Arbitrage/Disruptor-cpp/archive/master.zip\
 && unzip master.zip\
 && rm master.zip\
 && cd Disruptor-cpp-master\
 && mkdir build\
 && cd build\
 && cmake .. -DDISRUPTOR_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release\
 && make -j 2 install PREFIX=/usr/local\
 && cd ../../..

RUN mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j 2
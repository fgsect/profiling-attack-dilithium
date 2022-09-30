FROM fedora:34
RUN dnf update -y && dnf install -y vim gmp-devel clang perl cmake lldb wget
COPY ntl ntl/
ENV CC=clang
ENV CXX=clang++
RUN cd ntl/src/ && ./configure NTL_THREADS=on NTL_THREAD_BOOST=on && make && make install
RUN cd / && wget -c https://gitlab.com/libeigen/eigen/-/archive/3.4-rc1/eigen-3.4-rc1.tar.gz -O - | tar -xz && cd eigen-3.4-rc1 && mkdir build && cd build && cmake ../ && cmake --build . && make install
RUN dnf install -y coin-or-Cbc coin-or-Cbc-devel coin-or-CoinUtils-devel gmp-static glibc-static zlib-static abseil-cpp abseil-cpp-devel protobuf-devel git boost-devel readline-devel
COPY misc/scipoptsuite-7.0.3.tgz /scipoptsuite-7.0.3.tgz
RUN cd / && tar -xzvf /scipoptsuite-7.0.3.tgz  && cd scipoptsuite-7.0.3 && make -j80 && make install
RUN dnf install -y glog-devel
RUN wget -c https://github.com/google/or-tools/releases/download/v9.0/or-tools_fedora-33_v9.0.9048.tar.gz -O - | tar -xz && cd or-tools_Fedora-33-64bit_v9.0.9048/ && /usr/bin/cp -rf include/* /usr/local/include/ && cp lib/* /usr/local/lib && cp bin/* /usr/local/bin/
RUN dnf install -y armadillo-devel armadillo
#&& cd eigen-3.4-rc1 && mkdir build && cd build && cmake ../ && cmake --build . && make install
# cd / && wget -c https://github.com/google/or-tools/releases/download/v9.0/or-tools_fedora-33_v9.0.9048.tar.gz -O - | tar -xz && cd or-tools_Fedora-33-64bit_v9.0.9048/ && make install
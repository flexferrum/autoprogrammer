FROM ubuntu:16.04 as builder
MAINTAINER   Andrey Bronin <jonnib@yandex.ru>

RUN apt-get update && \
apt-get install -y --no-install-recommends \
apt-utils \
ca-certificates \
build-essential \
git \
llvm-dev \
make \
cmake \
libclang-5.0-dev \
llvm-5.0-dev \
libboost-dev \
libboost-iostreams-dev \
nano mc

RUN cd /home && git clone --depth=1 https://github.com/google/googletest.git &&\ 
cd googletest && cmake . && make install -j5

RUN git clone --recursive https://github.com/flexferrum/autoprogrammer.git /repo
RUN cd /repo && git checkout 1d246b809f916b0aa06388b985b17cc6f6b9e842

RUN cmake -H/repo -B/build
RUN cmake --build /build
RUN cmake --build /build --target test
RUN cmake --build /build --target install

CMD ["bash"]

#FROM scratch
#COPY --from=builder /build/fl-codegen /fl-codegen
#ENTRYPOINT ["/fl-codegen"]

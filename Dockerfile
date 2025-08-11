FROM --platform=linux/arm/v8 dtcooper/raspberrypi-os:bullseye AS build_armv8
LABEL org.opencontainers.image.source=https://github.com/monome/norns

# norns-ci container definition 
#
# This container is focused on creating an accurate build environment for CI. It does 
# not attempt to add any extra bits to make the container a usable way for a desktop
# user to interact with a running virtual norns environment in realtime. Emulating armv8 
# can be much slower (10-20x) than running on the host architecture.
#
# To build norns in a local armv8 container using a macOS or Windows machine with 
# Docker Desktop installed, run the following commands from the norns repo root folder. 
#
#    % docker build -t norns-ci . 
#    % docker run -v $(PWD):/norns-build --platform linux/arm64 -t norns-ci
#
# The norns-ci docker image is also published to the GitHub container registry. You can
# use the registry image directly to skip the container build step:
#
#    % docker run -v $(PWD):/norns-build --platform linux/arm64 -t ghcr.io/monome/norns-ci:latest
#
# You can also specify the registry as a cache source when building the container locally:
#
#    % docker build -t norns-ci . --cache-from ghcr.io/monome/norns-ci:latest
# 
# Building the container from Linux may require additional steps, see 
#    https://docs.docker.com/build/building/multi-platform/#install-qemu-manually
#
# Based on the instructions at
#    https://github.com/monome/norns-image/blob/main/build-dev-image.md
# and the Dockerfiles at
#    https://github.com/schollz/norns-desktop
#    https://github.com/winder/norns-dev
#    https://github.com/samdoshi/norns-dev
#
# note: packages not installed because of no package or no candidates: usbmount, midisport-firmware

ENV LANG=C.UTF-8 \
    DEBIAN_FRONTEND=noninteractive \
    LIBMONOME_VERSION=1.4.7 \
    LIBGPIOD_VERSION=1.6.4 \
    SUPERCOLLIDER_VERSION=3.13.0 \
    SUPERCOLLIDER_PLUGINS_VERSION=3.13.0

RUN apt-get update -yq && apt-get install -y \
    bc \
    build-essential \
    cmake \
    g++ \
    gcc \
    git \
    i2c-tools \
    libasound2-dev \
    libavahi-client-dev \
    libavahi-compat-libdnssd-dev \
    libboost-dev \
    libc6-dev \
    libcairo2-dev \
    libevdev-dev \
    libfftw3-dev \
    libjack-dev \
    liblo-dev \
    liblua5.3-dev \
    libnanomsg-dev \
    libncurses5-dev \
    libncursesw5-dev \
    libreadline-dev \
    libsamplerate0-dev \
    libsndfile1-dev \
    libudev-dev \
    libxt-dev \
    make \
    python-is-python3 \
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-wheel \
    vim \
    wget 
    
RUN apt-get install --no-install-recommends -y \
    dnsmasq-base \
    jackd2 \
    ladspalist \
    libjack-jackd2-dev \
    network-manager \
    samba

# Install libmonome
RUN cd /tmp/ && wget https://github.com/monome/libmonome/archive/v$LIBMONOME_VERSION.tar.gz -O libmonome.tar.gz && \
    tar -xvzf libmonome.tar.gz && cd /tmp/libmonome-$LIBMONOME_VERSION && \
    ./waf configure && \
    ./waf && ./waf install && \
    cd / && rm -rf /tmp/libmonome-$LIBMONOME_VERSION && ldconfig

# Install libgpiod
RUN mkdir -p /tmp/libgpiod && cd /tmp/libgpiod && \
    wget https://mirrors.edge.kernel.org/pub/software/libs/libgpiod/libgpiod-$LIBGPIOD_VERSION.tar.xz && \
    tar -xvf ./libgpiod-$LIBGPIOD_VERSION.tar.xz && \
    cd ./libgpiod-$LIBGPIOD_VERSION/ && \
    ./configure --enable-tools && \
    make && make install && cd /

# Install Supercollider
RUN mkdir -p /tmp/supercollider && cd /tmp/supercollider && \
    wget https://github.com/supercollider/supercollider/releases/download/Version-$SUPERCOLLIDER_VERSION/SuperCollider-$SUPERCOLLIDER_VERSION-Source.tar.bz2 -O sc.tar.bz2 && \
    tar xvf sc.tar.bz2 && cd /tmp/supercollider/SuperCollider-$SUPERCOLLIDER_VERSION-Source && \
    mkdir -p build && cd /tmp/supercollider/SuperCollider-$SUPERCOLLIDER_VERSION-Source/build && \
    cmake -DCMAKE_BUILD_TYPE="Release" \
          -DBUILD_TESTING=OFF \
          -DENABLE_TESTSUITE=OFF \
          -DNATIVE=OFF \
          -DINSTALL_HELP=OFF \
          -DSC_IDE=OFF \
          -DSC_QT=OFF \
          -DSC_ED=OFF \
          -DSC_EL=OFF \
          -DSUPERNOVA=OFF \
          -DSC_VIM=OFF \
          .. && \
    make -j1 && make install && cd /

# Install Supercollider plugins
RUN mkdir -p /tmp/sc3-plugins && cd /tmp/sc3-plugins && \
    git clone --depth=1 --recursive --branch Version-$SUPERCOLLIDER_PLUGINS_VERSION https://github.com/supercollider/sc3-plugins.git && \
    cd /tmp/sc3-plugins/sc3-plugins && mkdir -p build && \
    cd /tmp/sc3-plugins/sc3-plugins/build && \
    cmake -DSC_PATH=/tmp/supercollider/SuperCollider-$SUPERCOLLIDER_VERSION-Source \
          -DNATIVE=OFF \
          .. && \
    cmake --build . --config Release -- -j1 && \
    cmake --build . --config Release --target install && \
    cd / && ldconfig

# Suppress git errors due to ownership across host/container userids
RUN git config --global --add safe.directory /norns-build

# Support running the container with -i
ENV DEBIAN_FRONTEND=readline

RUN useradd -m we
USER we

WORKDIR /norns-build
ENTRYPOINT ["/bin/sh", "-c"]
CMD ["./waf configure --release && ./waf build --release"]
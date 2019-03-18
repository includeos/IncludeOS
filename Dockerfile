FROM ubuntu:18.04

ARG clang_version=6.0
RUN apt-get update && \
    apt-get -y install \
    clang-$clang_version \
    cmake \
    nasm \
    curl \
    git && \
    rm -rf /var/lib/apt/lists/*

# Install and configure Conan
ARG conan_version=1_12_3
RUN curl -Lo conan.deb https://dl.bintray.com/conan/installers/conan-ubuntu-64_$conan_version.deb && \
    dpkg --install conan.deb && \
    rm conan.deb
RUN conan config install https://github.com/includeos/conan_config.git && \
    conan config set general.default_profile=clang-$clang_version-linux-x86_64

# The files to be built must be hosted in a catalog called /service
# Default is to install conan dependencies and build
CMD mkdir -p /service/build && \
    cd /service/build && \
    conan install -g virtualenv .. && \
    . ./activate.sh && \
    cmake .. && \
    cmake --build .

from ubuntu:18.04

RUN apt-get update
RUN apt-get -y install \
    clang-6.0 \
    cmake \
    nasm \
    curl \
    git \
    && rm -rf /var/lib/apt/lists/*

# Install and configure Conan
ENV conan_version=1_12_3
RUN curl -Lo conan.deb https://dl.bintray.com/conan/installers/conan-ubuntu-64_$conan_version.deb \
    && dpkg --install conan.deb \
    && rm conan.deb
RUN conan config install https://github.com/includeos/conan_config.git

# Configure build environment
ENV CC=clang-6.0 \
    CXX=clang++-6.0
RUN mkdir service
WORKDIR service

CMD mkdir -p build && \
    cd build && \
    conan install .. -pr clang-6.0-linux-x86_64 && \
    cmake .. && \
    make

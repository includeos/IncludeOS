FROM ubuntu:xenial as base

RUN apt-get update && apt-get -y install \
    sudo \
    curl \
    locales \
    && rm -rf /var/lib/apt/lists/*
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

# Add fixuid to change permissions for bind-mounts. Set uid to same as host with -u <uid>:<guid>
RUN addgroup --gid 1000 docker && \
    adduser --uid 1000 --ingroup docker --home /home/docker --shell /bin/sh --disabled-password --gecos "" docker && \
    usermod -aG sudo docker && \
    sed -i.bkp -e \
      's/%sudo\s\+ALL=(ALL\(:ALL\)\?)\s\+ALL/%sudo ALL=NOPASSWD:ALL/g' \
      /etc/sudoers
RUN USER=docker && \
    GROUP=docker && \
    curl -SsL https://github.com/boxboat/fixuid/releases/download/v0.3/fixuid-0.3-linux-amd64.tar.gz | tar -C /usr/local/bin -xzf - && \
    chown root:root /usr/local/bin/fixuid && \
    chmod 4755 /usr/local/bin/fixuid && \
    mkdir -p /etc/fixuid && \
    printf "user: $USER\ngroup: $GROUP\npaths:\n  - /service\n" > /etc/fixuid/config.yml

RUN echo "LANG=C.UTF-8" > /etc/default/locale

# Docker TAG can be specified when building with --build-arg TAG=..., this is redeclared in the source-build stage
# Git tags can be specified with --build-arg NEWTAG=..., the default is set as includeos-dev
ARG BRANCH=dev
ARG REPO=hioa-cs
ARG NEWTAG=includeos-dev
ENV BRANCH=$BRANCH
ENV REPO=$REPO
ENV NEWTAG=$NEWTAG


LABEL dockerfile.version=1 \
      includeos.version=$BRANCH
WORKDIR /service

#########################
FROM base as source-build

RUN apt-get update && apt-get -y install \
    git \
    lsb-release \
    net-tools \
    wget \
    && rm -rf /var/lib/apt/lists/*


# Copy IncludeOS contents from host repo to container
RUN echo "copying Contents of current IncludeOS branch"
RUN cd ~ && pwd && \
  mkdir -p IncludeOS
COPY . /root/IncludeOS/

# Adding Custom git tags
RUN echo "Assigning Your custom git tag"
RUN cd /root/IncludeOS && \
    git tag -d $(git describe --tags) ; git tag $NEWTAG

# Installation
RUN cd /root/IncludeOS && \
    ./install.sh -n

RUN git -C /root/IncludeOS describe --dirty --tags > /ios_version.txt

#############################
FROM base as grubify

RUN apt-get update && apt-get -y install \
  dosfstools \
  grub-pc

COPY --from=source-build /usr/local/includeos/scripts/grubify.sh /home/ubuntu/IncludeOS_install/includeos/scripts/grubify.sh

ENTRYPOINT ["fixuid", "/home/ubuntu/IncludeOS_install/includeos/scripts/grubify.sh"]

###########################
FROM base as build

RUN apt-get update && apt-get -y install \
    git \
    clang-5.0 \
    cmake \
    nasm \
    python-pip \
    && rm -rf /var/lib/apt/lists/* \
    && pip install pystache antlr4-python2-runtime && \
    apt-get remove -y python-pip && \
    apt autoremove -y

COPY --from=source-build  /usr/local/includeos /usr/local/includeos/
COPY --from=source-build  /usr/local/bin/boot /usr/local/bin/boot
COPY --from=source-build  /root/IncludeOS/etc/install_dependencies_linux.sh /
COPY --from=source-build  /root/IncludeOS/etc/use_clang_version.sh /
COPY --from=source-build  /root/IncludeOS/lib/uplink/starbase /root/IncludeOS/lib/uplink/starbase/
COPY --from=source-build  /ios_version.txt /
COPY --from=source-build  /root/IncludeOS/etc/docker_entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]

CMD mkdir -p build && \
  cd build && \
  cp $(find /usr/local/includeos -name chainloader) /service/build/chainloader && \
  cmake .. && \
  make

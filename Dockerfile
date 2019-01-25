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
RUN echo "LANG=C.UTF-8" > /etc/default/locale

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

ARG VCSREF
ARG VERSION

LABEL org.label-schema.schema-version="1.0" \
      org.label-schema.vcs-url="https://github.com/hioa-cs/includeos" \
      org.label-schema.vendor="IncludeOS" \
      org.label-schema.vcs-ref=$VCSREF \
      org.label-schema.version=$VERSION

#########################
FROM base as source-build

RUN apt-get update && apt-get -y install \
    git \
    lsb-release \
    net-tools \
    wget \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /root/IncludeOS
WORKDIR /root/IncludeOS
COPY . .

# Ability to specify custom tag that overwrites any existing tag. This will then match what IncludeOS reports itself.
ARG VERSION
RUN git describe --tags --dirty > /ios_version.txt
RUN echo ${VERSION:=$(git describe --tags --dirty)} && git tag -d $(git describe --tags); git tag $VERSION && git describe --tags --dirty > /version.txt

# Installation
RUN ./install.sh -n

#############################
FROM base as grubify

RUN apt-get update && apt-get -y install \
  dosfstools \
  grub-pc

LABEL org.label-schema.description="Add a grub bootloader to any binary" \
      org.label-schema.name="IncludeOS_grubify"

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

# Metadata used for labels
ARG BUILDDATE

LABEL org.label-schema.description="Build a service using IncludeOS" \
      org.label-schema.name="IncludeOS_builder" \
      org.label-schema.docker.cmd="docker run -v $PWD:/service <container>" \
      org.label-schema.build-date=$BUILDDATE

WORKDIR /service

COPY --from=source-build  /usr/local/includeos /usr/local/includeos/
COPY --from=source-build  /root/IncludeOS/etc/use_clang_version.sh /
COPY --from=source-build  /root/IncludeOS/lib/uplink/starbase /root/IncludeOS/lib/uplink/starbase/
COPY --from=source-build  /ios_version.txt /
COPY --from=source-build  /version.txt /
COPY --from=source-build  /root/IncludeOS/etc/docker_entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]

CMD mkdir -p build && \
  cd build && \
  cp $(find /usr/local/includeos -name chainloader) /service/build/chainloader && \
  echo "IncludeOS reported version:" $(cat /ios_version.txt) "label Version:" $(cat /version.txt) && \
  cmake .. && \
  make

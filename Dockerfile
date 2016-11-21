FROM ubuntu:xenial

RUN apt-get update && apt-get -y install \
    git \
    net-tools \
    sudo \
    wget \
&& rm -rf /var/lib/apt/lists/*

RUN useradd --create-home -s /bin/bash ubuntu
RUN adduser ubuntu sudo
RUN echo -n 'ubuntu:ubuntu' | chpasswd

# Enable passwordless sudo for users under the "sudo" group
RUN sed -i.bkp -e \
      's/%sudo\s\+ALL=(ALL\(:ALL\)\?)\s\+ALL/%sudo ALL=NOPASSWD:ALL/g' \
      /etc/sudoers

USER ubuntu

ADD . /home/ubuntu/IncludeOS
WORKDIR /home/ubuntu/IncludeOS

RUN sudo apt-get update && \
    sudo do_bridge="" ./etc/install_all_source.sh \
&& sudo rm -rf /var/lib/apt/lists/*

VOLUME /service
WORKDIR /service

CMD ./run.sh

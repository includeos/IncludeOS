#
# Prerequisites for building VirtualBox from source
#
# Target platform: Ubuntu 14.04 x86_64
#
# Based on https://www.virtualbox.org/wiki/Linux%20build%20instructions
# ...but lots of this was added by following error messages from configure/kmk

apt-get update

# Standard stuff (It needs Texlive - omg!)
apt-get install -y gcc g++ bcc iasl xsltproc uuid-dev zlib1g-dev libidl-dev \
                libsdl1.2-dev libxcursor-dev libasound2-dev libstdc++5 \
                libhal-dev libpulse-dev libxml2-dev libxslt1-dev \
                python-dev libqt4-dev qt4-dev-tools libcap-dev \
                libxmu-dev mesa-common-dev libglu1-mesa-dev \
                linux-kernel-headers libcurl4-openssl-dev libpam0g-dev \
                libxrandr-dev libxinerama-dev libqt4-opengl-dev makeself \
                libdevmapper-dev default-jdk python-central \
                texlive-latex-base \
                texlive-latex-extra texlive-latex-recommended \
                texlive-fonts-extra texlive-fonts-recommended

# To get ia32-libs 
cd /etc/apt/sources.list.d
echo "deb http://old-releases.ubuntu.com/ubuntu/ raring main restricted universe multiverse" >ia32-libs-raring.list

apt-get install -y ia32-libs

# 64-bit specific (I think)
apt-get install -y libc6-dev-i386 
apt-get install -y lib32gcc1 gcc-multilib 
apt-get install -y lib32stdc++6 
apt-get install -y g++-multilib
apt-get install -y subversion
apt-get install -y libvpx-dev

# ...Yes, it needs old java 6
apt-get install -y openjdk-6-jdk

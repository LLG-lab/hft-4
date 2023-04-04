FROM ubuntu:22.04
ARG BOOST_DIR=boost_1_81_0
ARG BOOST_TARBALL=boost_1_81_0.tar.bz2
ARG BOOST_URL=https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/$BOOST_TARBALL
RUN echo 'APT::Install-Suggests "0";' >> /etc/apt/apt.conf.d/00-docker
RUN echo 'APT::Install-Recommends "0";' >> /etc/apt/apt.conf.d/00-docker
RUN DEBIAN_FRONTEND=noninteractive \
  apt-get update && \
  apt-get install -y \
      bzip2 \
      mc \
      wget \
      cmake \
      make \
      libcurl4-openssl-dev \
      libssl-dev \
      libprotobuf-dev \
      protobuf-compiler \
      dpkg-dev \
      g++ && \
  cd /root/ && \
  wget --no-check-certificate $BOOST_URL && \
  tar -jxf $BOOST_TARBALL && \
  rm -f $BOOST_TARBALL && \
  cd $BOOST_DIR && \
  ./bootstrap.sh && \
  ./b2 --build-dir=build-directory toolset=gcc cxxflags=-std=gnu++0x stage && \
  mkdir /root/exchange && \
  echo '#!/bin/bash' > /root/deb-generator && \
  echo -ne "\n[ -f /root/exchange/deb-generator.sh ] || {\n    echo \"Failed to load file ‘/root/exchange/deb-generator.sh’\"\n    exit 1\n}\n\nsource /root/exchange/deb-generator.sh" >> /root/deb-generator && \
  chmod +x /root/deb-generator
VOLUME ["/root/exchange"]
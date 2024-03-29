FROM centos:centos7
ARG BOOST_DIR=boost_1_81_0
ARG BOOST_TARBALL=boost_1_81_0.tar.bz2
ARG BOOST_URL=https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/$BOOST_TARBALL
RUN yum -y update
RUN yum -y install centos-release-scl
RUN yum -y install \
      wget \
      make \
      git  \
      rpm-build \
      rpmdevtools \
      openssl-devel \
      python-devel \
      curl-devel \
      sqlite-devel \
      newt-devel \
      maven \
      protobuf-devel \
      protobuf-compiler \
      devtoolset-7-gcc* && \
    pushd /tmp > /dev/null && \
    wget https://github.com/Kitware/CMake/releases/download/v3.17.3/cmake-3.17.3.tar.gz && \
    tar -zxf cmake-3.17.3.tar.gz && \
    cd cmake-3.17.3 && \
    scl enable devtoolset-7 "./bootstrap --prefix=/usr/" && \
    scl enable devtoolset-7 make && \
    make install && \
    cd ../ && \
    rm -rf cmake-3.17.3 && \
    rm -f cmake-3.17.3.tar.gz && \
    popd > /dev/null && \
    pushd /root/ && \
    wget $BOOST_URL && \
    tar -jxf $BOOST_TARBALL && \
    rm -f $BOOST_TARBALL && \
    pushd $BOOST_DIR > /dev/null && \
    scl enable devtoolset-7 "./bootstrap.sh" && \
    popd > /dev/null && \
    pushd $BOOST_DIR > /dev/null && \
    scl enable devtoolset-7 "./b2 --build-dir=build-directory toolset=gcc cxxflags=-std=gnu++0x stage" && \
    popd > /dev/null && \
    popd && \
    rpmdev-setuptree && \
    mkdir /root/exchange && \
    echo '#!/bin/bash -e' > /root/rpm-generator && \
    echo -ne "\n[ -f /root/exchange/rpm-generator.sh ] || {\n    echo \"Failed to load file ‘/root/exchange/rpm-generator.sh’\"\n    exit 1\n}\n\nsource /root/exchange/rpm-generator.sh" >> /root/rpm-generator && \
    chmod +x /root/rpm-generator
VOLUME ["/root/exchange"]
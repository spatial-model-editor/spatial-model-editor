#!/bin/bash

# install common linux CI dependencies

set -e -x

# add llvm repo for clang / llvm-cov 14
sudo wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
sudo add-apt-repository "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-14 main"

sudo apt-get update -yy

# install build/test dependencies
sudo apt-get install -yy \
    ccache \
    xvfb \
    jwm \
    lcov \
    clang-14 \
    llvm-14

# install qt build dependencies
sudo apt-get install -yy \
    libglu1-mesa-dev \
    libx11-dev \
    libx11-xcb-dev \
    libxext-dev \
    libxfixes-dev \
    libxi-dev \
    libxrender-dev \
    libxcb1-dev \
    libxcb-glx0-dev \
    libxcb-keysyms1-dev \
    libxcb-image0-dev \
    libxcb-shm0-dev \
    libxcb-icccm4-dev \
    libxcb-sync-dev \
    libxcb-xfixes0-dev \
    libxcb-shape0-dev \
    libxcb-randr0-dev \
    libxcb-render-util0-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    '^libxcb.*-dev'

# use clang 14 as default version
sudo update-alternatives --remove-all clang || echo "nothing to remove"
sudo update-alternatives --remove-all clang++ || echo "nothing to remove"
sudo update-alternatives --remove-all llvm-cov || echo "nothing to remove"
sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-14 100
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-14 100
sudo update-alternatives --install /usr/bin/llvm-cov llvm-cov /usr/bin/llvm-cov-14 100

# get a reasonably recent version of ccache from conda-forge
# nb: more recent versions from conda-forge depend on libhiredis >= 1 which is not available on ubuntu
wget https://anaconda.org/conda-forge/ccache/4.3/download/linux-64/ccache-4.3-haef5404_1.tar.bz2
tar -xf ccache-4.3-haef5404_1.tar.bz2
sudo cp bin/ccache /usr/bin/ccache

# check versions
cmake --version
g++ --version
gcov --version
clang++ --version
llvm-cov --version
clang-format --version
python --version
ccache --version

# set maximum ccache size to 400MB
ccache --max-size 400M
ccache --cleanup
ccache --zero-stats
ccache --show-stats

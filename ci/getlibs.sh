#!/bin/bash

# bash script to download static libs
# usage: ./ci/getlibs.sh [linux, osx, win32, win64]

set -e -x

OS=$1
echo "Downloading static libs for OS: $OS"
# download static libs
for LIB in qt5 common dune
do
    wget "https://github.com/spatial-model-editor/sme_deps_${LIB}/releases/latest/download/sme_deps_${LIB}_${OS}.tgz"
    tar xvf sme_deps_${LIB}_${OS}.tgz
done
pwd
ls
# copy libs to desired location: workaround for tar -C / not working on windows
if [[ "$OS" == *"win"* ]]; then
   mv smelibs /c/
   ls /c/smelibs
else
   mv opt/* /opt/
   ls /opt/smelibs
fi

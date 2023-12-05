#!/bin/bash

# bash script to download static libs
# usage: ./ci/getlibs.sh [linux, osx, win32, win64]

SME_DEPS_VERSION="2023.12.12"
OS=$1

set -e -x

echo "Downloading sme_deps ${SME_DEPS_VERSION} for ${OS}"
wget "https://github.com/spatial-model-editor/sme_deps/releases/download/${SME_DEPS_VERSION}/sme_deps_${OS}.tgz"
tar xf sme_deps_"$OS".tgz
# copy libs to desired location: workaround for tar -C / not working on windows
if [[ "$OS" == *"win"* ]]; then
    mv c/smelibs /c/
    # ls /c/smelibs
else
    mv opt/* /opt/
    # ls /opt/smelibs
fi

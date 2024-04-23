#!/bin/bash

# bash script to download static libs on CI

SME_DEPS_VERSION="2024.04.24"

set -e -x

if [[ "$RUNNER_OS" == "Linux" ]]; then
    OS="linux"
elif [[ "$RUNNER_OS" == "macOS" ]] && [[ "$RUNNER_ARCH" == "X64" ]]; then
    OS="osx"
elif [[ "$RUNNER_OS" == "macOS" ]] && [[ "$RUNNER_ARCH" == "ARM64" ]]; then
    OS="osx-arm64"
elif [[ "$RUNNER_OS" == "Windows" ]]; then
    OS="win64-mingw"
fi

echo "Downloading sme_deps ${SME_DEPS_VERSION} for ${OS}"
wget "https://github.com/spatial-model-editor/sme_deps/releases/download/${SME_DEPS_VERSION}/sme_deps_${OS}.tgz"
tar xf sme_deps_"$OS".tgz
# copy libs to desired location: workaround for tar -C / not working on msys2
if [[ "$OS" == "win64-mingw" ]]; then
    mv c/smelibs /c/
else
    ${SUDO_CMD} mv opt/* /opt/
fi

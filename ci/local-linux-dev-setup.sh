#!/usr/bin/env bash
set -euo pipefail

# A script to set up a local linux dev environment with dependencies and a release & debug build

# Run this from the top level directory of the repository.

log() {
    echo "[local-linux-dev-setup] $*"
}

print_help() {
    cat <<'EOF'
Set up a local Linux dev environment with SME dependencies and builds.

By default this script:
1. Downloads and installs SME dependency libraries into /opt/smelibs
2. Configures and builds a release build in build-sme-release
3. Configures and builds an ASAN build in build-sme-asan

WARNING: this script creates or overwrites:
- /opt/smelibs
- build-sme-release
- build-sme-asan

Usage:
  ci/local-linux-dev-setup.sh [--deps-only | --skip-deps | --help]

Options:
  --deps-only   Only download/install dependencies to /opt/smelibs
  --skip-deps   Skip dependency download/install and run both builds
  --help        Show this help message
EOF
}

RUN_DEPS=1
RUN_BUILDS=1
for arg in "$@"; do
    case "$arg" in
    --help | -h)
        print_help
        exit 0
        ;;
    --deps-only)
        RUN_DEPS=1
        RUN_BUILDS=0
        ;;
    --skip-deps)
        RUN_DEPS=0
        RUN_BUILDS=1
        ;;
    *)
        echo "Error: unknown option: $arg"
        print_help
        exit 1
        ;;
    esac
done

if [ "$RUN_DEPS" -eq 0 ] && [ "$RUN_BUILDS" -eq 0 ]; then
    echo "Error: nothing to do with this option combination."
    exit 1
fi

check_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Error: required command not found: $1"
        exit 1
    fi
}

cleanup() {
    rm -f sme_deps_linux.tgz
}
trap cleanup EXIT

check_cmd sudo
check_cmd tar
check_cmd cmake
check_cmd ninja
check_cmd clang
check_cmd clang++
check_cmd ccache
check_cmd git
if [ "$RUN_DEPS" -eq 1 ]; then
    if ! command -v wget >/dev/null 2>&1 && ! command -v curl >/dev/null 2>&1; then
        echo "Error: required download tool not found (need wget or curl)"
        exit 1
    fi
fi

repo_root="$(git rev-parse --show-toplevel 2>/dev/null || true)"
if [ -z "$repo_root" ] || [ "$PWD" != "$repo_root" ]; then
    echo "Error: run this script from the top-level directory of the repository."
    exit 1
fi
if [ ! -f "ci/local-linux-dev-setup.sh" ]; then
    echo "Error: expected to find ci/local-linux-dev-setup.sh in current directory."
    exit 1
fi

log "WARNING: this script creates or overwrites: /opt/smelibs, build-sme-release, build-sme-asan"
log "Refreshing sudo credentials"
sudo -v

download_deps() {
    if command -v wget >/dev/null 2>&1; then
        wget -O sme_deps_linux.tgz https://github.com/spatial-model-editor/sme_deps/releases/latest/download/sme_deps_linux.tgz
    else
        curl -L -o sme_deps_linux.tgz https://github.com/spatial-model-editor/sme_deps/releases/latest/download/sme_deps_linux.tgz
    fi
}

# download the static libraries as used by linux CI builds and install to /opt/smelibs
if [ "$RUN_DEPS" -eq 1 ]; then
    log "Removing existing /opt/smelibs"
    sudo rm -rf /opt/smelibs
    log "Downloading dependency archive (sme_deps_linux.tgz)"
    rm -f sme_deps_linux.tgz
    download_deps
    log "Extracting dependency archive to /"
    sudo tar xf sme_deps_linux.tgz -C /
    log "Setting permissions on /opt/smelibs"
    sudo chmod -R 755 /opt/smelibs
fi

# make a build-sme-release directory for a RELEASE build
if [ "$RUN_BUILDS" -eq 1 ]; then
    log "Configuring release build in build-sme-release"
    mkdir -p build-sme-release
    rm -rf -- build-sme-release/*
    CC=clang CXX=clang++ cmake -GNinja \
        -S . \
        -B build-sme-release \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
        -DCMAKE_CXX_FLAGS="-fpic -fvisibility=hidden -DNQT_DEBUG" \
        -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake" \
        -DCMAKE_INSTALL_PREFIX=/opt/smelibs \
        -DEXPAT_INCLUDE_DIR=/opt/smelibs/include \
        -DEXPAT_LIBRARY=/opt/smelibs/lib/libexpat.a \
        -DSME_LOG_LEVEL=INFO \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    log "Building release target with Ninja"
    ninja -C build-sme-release
fi

# make a build-asan directory for an ASAN build for debugging segfaults
if [ "$RUN_BUILDS" -eq 1 ]; then
    log "Configuring ASAN build in build-sme-asan"
    mkdir -p build-sme-asan
    rm -rf -- build-sme-asan/*
    CC=clang CXX=clang++ cmake -GNinja \
        -S . \
        -B build-sme-asan \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer" \
        -DCMAKE_CXX_FLAGS="-fpic -fvisibility=hidden -DNQT_DEBUG -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer -Wall" \
        -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake" \
        -DCMAKE_INSTALL_PREFIX=/opt/smelibs \
        -DEXPAT_INCLUDE_DIR=/opt/smelibs/include \
        -DEXPAT_LIBRARY=/opt/smelibs/lib/libexpat.a \
        -DSME_LOG_LEVEL=INFO \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    log "Building ASAN target with Ninja"
    ASAN_OPTIONS="verbosity=0:halt_on_error=0" ninja -C build-sme-asan
fi

log "Completed successfully"

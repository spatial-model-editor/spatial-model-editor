#!/usr/bin/env bash
set -euo pipefail

INSTALL_PREFIX="/opt/smelibs"
BUILD_RELEASE_DIR="build-release"
BUILD_ASAN_DIR="build-asan"
CMAKE_PREFIX_PATH_VALUE="${INSTALL_PREFIX};${INSTALL_PREFIX}/lib/cmake"

HOST_OS="$(uname -s)"
HOST_ARCH="$(uname -m)"
PLATFORM=""
DEPS_ARCHIVE=""
DEPS_URL=""
MACOS_DEPLOYMENT_TARGET=""

log() {
    echo "[local-dev-setup] $*"
}

print_help() {
    cat <<'EOF'
Set up a local SME dev environment with prebuilt dependencies and builds.

By default this script:
1. Downloads and installs the appropriate SME dependency archive into /opt/smelibs
2. Configures and builds a release build in build-release
3. Configures and builds an ASAN build in build-asan

Supported dependency archives:
- Linux x86_64: sme_deps_linux.tgz
- Linux arm64/aarch64: sme_deps_linux-arm64.tgz
- macOS arm64: sme_deps_osx-arm64.tgz

WARNING: this script creates or overwrites:
- /opt/smelibs
- build-release
- build-asan

Usage:
  ci/local-dev-setup.sh [--deps-only | --skip-deps | --help]

Options:
  --deps-only   Only download/install dependencies to /opt/smelibs
  --skip-deps   Skip dependency download/install and run both builds
  --help        Show this help message
EOF
}

die() {
    echo "Error: $*" >&2
    exit 1
}

check_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        die "required command not found: $1"
    fi
}

detect_platform() {
    MACOS_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-}"
    case "$HOST_OS" in
    Linux)
        PLATFORM="linux"
        case "$HOST_ARCH" in
        x86_64)
            DEPS_ARCHIVE="sme_deps_linux.tgz"
            ;;
        arm64 | aarch64)
            DEPS_ARCHIVE="sme_deps_linux-arm64.tgz"
            ;;
        esac
        ;;
    Darwin)
        PLATFORM="macos"
        DEPS_ARCHIVE="sme_deps_osx-arm64.tgz"
        if [ -z "$MACOS_DEPLOYMENT_TARGET" ] && command -v sw_vers >/dev/null 2>&1; then
            MACOS_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-$(sw_vers -productVersion | cut -d. -f1-2)}"
        fi
        ;;
    *)
        die "unsupported host OS: $HOST_OS"
        ;;
    esac

    if [ -n "$DEPS_ARCHIVE" ]; then
        DEPS_URL="https://github.com/spatial-model-editor/sme_deps/releases/latest/download/${DEPS_ARCHIVE}"
    fi
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
        print_help
        die "unknown option: $arg"
        ;;
    esac
done

if [ "$RUN_DEPS" -eq 0 ] && [ "$RUN_BUILDS" -eq 0 ]; then
    die "nothing to do with this option combination"
fi

detect_platform
if [ "$RUN_DEPS" -eq 1 ] && [ -z "$DEPS_ARCHIVE" ]; then
    die "no published sme_deps archive is available for ${HOST_OS} ${HOST_ARCH}"
fi

cleanup() {
    if [ -n "$DEPS_ARCHIVE" ]; then
        rm -f "$DEPS_ARCHIVE"
    fi
}
trap cleanup EXIT

check_cmd git
if [ "$RUN_DEPS" -eq 1 ]; then
    check_cmd sudo
    check_cmd tar
    if ! command -v wget >/dev/null 2>&1 && ! command -v curl >/dev/null 2>&1; then
        die "required download tool not found (need wget or curl)"
    fi
fi
if [ "$RUN_BUILDS" -eq 1 ]; then
    check_cmd cmake
    check_cmd ninja
    check_cmd clang
    check_cmd clang++
    check_cmd ccache
fi

repo_root="$(git rev-parse --show-toplevel 2>/dev/null || true)"
if [ -z "$repo_root" ] || [ "$PWD" != "$repo_root" ]; then
    die "run this script from the top-level directory of the repository"
fi
if [ ! -f "ci/local-dev-setup.sh" ]; then
    die "expected to find ci/local-dev-setup.sh in current directory"
fi

warning_targets=""
append_warning_target() {
    if [ -n "$warning_targets" ]; then
        warning_targets="$warning_targets, $1"
    else
        warning_targets="$1"
    fi
}

if [ "$RUN_DEPS" -eq 1 ]; then
    append_warning_target "$INSTALL_PREFIX"
fi
if [ "$RUN_BUILDS" -eq 1 ]; then
    append_warning_target "$BUILD_RELEASE_DIR"
    append_warning_target "$BUILD_ASAN_DIR"
fi

log "Host platform: ${HOST_OS} ${HOST_ARCH}"
if [ -n "$DEPS_ARCHIVE" ]; then
    log "Selected dependency archive: ${DEPS_ARCHIVE}"
fi
if [ -n "$MACOS_DEPLOYMENT_TARGET" ]; then
    log "Using macOS deployment target: ${MACOS_DEPLOYMENT_TARGET}"
fi
log "WARNING: this script creates or overwrites: $warning_targets"
if [ "$RUN_DEPS" -eq 1 ]; then
    log "Refreshing sudo credentials"
    sudo -v
fi

download_deps() {
    if command -v wget >/dev/null 2>&1; then
        wget -O "$DEPS_ARCHIVE" "$DEPS_URL"
    else
        curl -L -o "$DEPS_ARCHIVE" "$DEPS_URL"
    fi
}

reset_build_dir() {
    local build_dir="$1"
    mkdir -p "$build_dir"
    rm -rf -- "${build_dir}"/*
}

configure_build() {
    local build_dir="$1"
    local cxx_flags="$2"
    local linker_flags="$3"
    local -a cmake_args=(
        -GNinja
        -S .
        -B "$build_dir"
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
        -DCMAKE_BUILD_TYPE=RelWithDebInfo
        -DBUILD_SHARED_LIBS=OFF
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
        -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH_VALUE"
        -DCMAKE_CXX_FLAGS="$cxx_flags"
        -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOS_DEPLOYMENT_TARGET"
        -DCUDAToolkit_ROOT="${INSTALL_PREFIX}/cuda"
        -DEXPAT_INCLUDE_DIR="${INSTALL_PREFIX}/include"
        -DEXPAT_LIBRARY="${INSTALL_PREFIX}/lib/libexpat.a"
        -DSME_LOG_LEVEL=INFO
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    )
    if [ "$PLATFORM" = "macos" ]; then
        cmake_args+=(
            -DFREETYPE_LIBRARY_RELEASE="${INSTALL_PREFIX}/lib/libQt6BundledFreetype.a"
            -DFREETYPE_INCLUDE_DIR_freetype2="${INSTALL_PREFIX}/include/QtFreetype"
            -DFREETYPE_INCLUDE_DIR_ft2build="${INSTALL_PREFIX}/include/QtFreetype"
        )
    fi
    if [ -n "$linker_flags" ]; then
        cmake_args+=(-DCMAKE_EXE_LINKER_FLAGS="$linker_flags")
    fi
    CC=clang CXX=clang++ cmake "${cmake_args[@]}"
}

build_release() {
    log "Configuring release build in ${BUILD_RELEASE_DIR}"
    reset_build_dir "$BUILD_RELEASE_DIR"
    case "$PLATFORM" in
    linux)
        configure_build "$BUILD_RELEASE_DIR" "-fpic -fvisibility=hidden -DNQT_DEBUG" "-fuse-ld=lld"
        ;;
    macos)
        configure_build "$BUILD_RELEASE_DIR" "-DNDEBUG -fpic -fvisibility=hidden" ""
        ;;
    esac
    log "Building release target with Ninja"
    ninja -C "$BUILD_RELEASE_DIR"
}

build_asan() {
    local cxx_flags=""
    local linker_flags=""
    log "Configuring ASAN build in ${BUILD_ASAN_DIR}"
    reset_build_dir "$BUILD_ASAN_DIR"
    case "$PLATFORM" in
    linux)
        cxx_flags="-fpic -fvisibility=hidden -DNQT_DEBUG -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer -Wall"
        linker_flags="-fuse-ld=lld -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer"
        ;;
    macos)
        cxx_flags="-DNDEBUG -fpic -fvisibility=hidden -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer -Wall"
        linker_flags="-fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer"
        ;;
    esac
    configure_build "$BUILD_ASAN_DIR" "$cxx_flags" "$linker_flags"
    log "Building ASAN target with Ninja"
    ASAN_OPTIONS="verbosity=0:halt_on_error=0" ninja -C "$BUILD_ASAN_DIR"
}

if [ "$RUN_DEPS" -eq 1 ]; then
    log "Removing existing ${INSTALL_PREFIX}"
    sudo rm -rf "$INSTALL_PREFIX"
    log "Downloading dependency archive (${DEPS_ARCHIVE})"
    rm -f "$DEPS_ARCHIVE"
    download_deps
    log "Extracting dependency archive to /"
    sudo tar xf "$DEPS_ARCHIVE" -C /
    log "Setting permissions on ${INSTALL_PREFIX}"
    sudo chmod -R 755 "$INSTALL_PREFIX"
fi

if [ "$RUN_BUILDS" -eq 1 ]; then
    build_release
    build_asan
fi

log "Completed successfully"

#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TOOLCHAIN_FILE="$PROJECT_ROOT/cmake/x86_64-elf-toolchain.cmake"

usage() {
    echo "Usage: $0 [clean|configure|build|iso|run|all]"
    echo "  clean       - remove build directory"
    echo "  configure   - run cmake configure step"
    echo "  build       - build the OS"
    echo "  iso         - build the bootable ISO"
    echo "  run         - build ISO and launch QEMU"
    echo "  all         - clean, configure, and run"
    exit 1
}

do_clean() {
    echo "==> Cleaning build directory"
    rm -rf "$BUILD_DIR"
}

do_configure() {
    if [ ! -d "$BUILD_DIR" ]; then
        mkdir -p "$BUILD_DIR"
    fi
    echo "==> Configuring project"
    cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
}

do_build() {
    echo "==> Building OS components"
    cmake --build "$BUILD_DIR"
}

do_iso() {
    echo "==> Building ISO"
    cmake --build "$BUILD_DIR" --target iso
}

do_run() {
    echo "==> Launching QEMU"
    cmake --build "$BUILD_DIR" --target run
}

CMD="${1:-all}"

case "$CMD" in
    clean)      do_clean ;;
    configure)  do_configure ;;
    build)      do_configure && do_build ;;
    iso)        do_configure && do_iso ;;
    run)        do_configure && do_run ;;
    all)        do_clean && do_configure && do_run ;;
    *)          usage ;;
esac
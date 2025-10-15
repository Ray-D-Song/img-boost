#!/bin/bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

echo_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
THIRD_PARTY_DIR="${PROJECT_ROOT}/third_party"
BUILD_DIR="${PROJECT_ROOT}/build_deps"
PREFIX="${THIRD_PARTY_DIR}"

mkdir -p "${BUILD_DIR}"
mkdir -p "${PREFIX}"

# Library version
LIBPNG_VERSION="1.6.43"
LIBJPEG_TURBO_VERSION="2.1.5.1"
LIBWEBP_VERSION="1.3.2"

NPROC=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

cd "${BUILD_DIR}"

# zlib (dep by libpng)
if [ ! -f "${PREFIX}/lib/libz.a" ]; then
    echo_info "Building zlib..."
    if [ ! -d "zlib" ]; then
        git clone --depth 1 --branch v1.3.1 https://github.com/madler/zlib.git
    fi
    cd zlib
    ./configure --prefix="${PREFIX}" --static
    make -j${NPROC}
    make install
    cd ..
else
    echo_info "zlib: skipped (exists)"
fi

# libpng
if [ ! -f "${PREFIX}/lib/libpng.a" ]; then
    echo_info "Building libpng ${LIBPNG_VERSION}..."
    if [ ! -d "libpng-${LIBPNG_VERSION}" ]; then
        curl -L "https://download.sourceforge.net/libpng/libpng-${LIBPNG_VERSION}.tar.gz" -o libpng.tar.gz
        tar xzf libpng.tar.gz
    fi
    cd "libpng-${LIBPNG_VERSION}"
    ./configure --prefix="${PREFIX}" --enable-static --disable-shared \
        --disable-hardware-optimizations \
        CPPFLAGS="-I${PREFIX}/include" LDFLAGS="-L${PREFIX}/lib"
    make -j${NPROC}
    make install
    cd ..
else
    echo_info "libpng: skipped (exists)"
fi

# libjpeg-turbo
if [ ! -f "${PREFIX}/lib/libjpeg.a" ]; then
    echo_info "Building libjpeg-turbo ${LIBJPEG_TURBO_VERSION}..."
    if [ ! -d "libjpeg-turbo-${LIBJPEG_TURBO_VERSION}" ]; then
        curl -L "https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/${LIBJPEG_TURBO_VERSION}/libjpeg-turbo-${LIBJPEG_TURBO_VERSION}.tar.gz" -o libjpeg-turbo.tar.gz
        tar xzf libjpeg-turbo.tar.gz
    fi
    cd "libjpeg-turbo-${LIBJPEG_TURBO_VERSION}"
    mkdir -p build && cd build
    cmake .. -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
        -DENABLE_SHARED=OFF \
        -DENABLE_STATIC=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    make -j${NPROC}
    make install
    cd ../..
else
    echo_info "libjpeg-turbo: skipped (exists)"
fi

# libwebp
if [ ! -f "${PREFIX}/lib/libwebp.a" ]; then
    echo_info "Building libwebp ${LIBWEBP_VERSION}..."
    if [ ! -d "libwebp-${LIBWEBP_VERSION}" ]; then
        curl -L "https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-${LIBWEBP_VERSION}.tar.gz" -o libwebp.tar.gz
        tar xzf libwebp.tar.gz
    fi
    cd "libwebp-${LIBWEBP_VERSION}"
    mkdir -p build && cd build
    cmake .. -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
        -DBUILD_SHARED_LIBS=OFF \
        -DWEBP_BUILD_ANIM_UTILS=OFF \
        -DWEBP_BUILD_CWEBP=OFF \
        -DWEBP_BUILD_DWEBP=OFF \
        -DWEBP_BUILD_GIF2WEBP=OFF \
        -DWEBP_BUILD_IMG2WEBP=OFF \
        -DWEBP_BUILD_VWEBP=OFF \
        -DWEBP_BUILD_WEBPINFO=OFF \
        -DWEBP_BUILD_WEBPMUX=OFF \
        -DWEBP_BUILD_EXTRAS=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        -DPNG_PNG_INCLUDE_DIR="${PREFIX}/include" \
        -DPNG_LIBRARY="${PREFIX}/lib/libpng.a" \
        -DJPEG_INCLUDE_DIR="${PREFIX}/include" \
        -DJPEG_LIBRARY="${PREFIX}/lib/libjpeg.a"
    make -j${NPROC}
    make install
    cd ../..
else
    echo_info "libwebp: skipped (exists)"
fi

echo_info "Done. Libraries installed to: ${PREFIX}/lib"

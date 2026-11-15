#!/usr/bin/env bash
set -euo pipefail

if ! command -v apt-get >/dev/null 2>&1; then
    echo "This script requires an apt-based Linux distribution (Ubuntu/Debian)." >&2
    exit 1
fi

if (( EUID == 0 )); then
    apt_get=(apt-get)
else
    if ! command -v sudo >/dev/null 2>&1; then
        echo "Please run this script as root or install sudo." >&2
        exit 1
    fi
    apt_get=(sudo apt-get)
fi

packages=(
    build-essential
    cmake
    git
    ninja-build
    pkg-config
    libasound2-dev
    libavcodec-dev
    libavformat-dev
    libavutil-dev
    libboost-dev
    libcurl4-openssl-dev
    libgtk-3-dev
    libgif-dev
    libglu1-mesa-dev
    libharfbuzz-dev
    libmpg123-dev
    libopencv-dev
    libopus-dev
    libopusfile-dev
    libsoundtouch-dev
    libswresample-dev
    libtiff-dev
    libturbojpeg0-dev
    libvorbis-dev
    libwebp-dev
    libxft-dev
    uuid-dev
    xorg-dev
)

"${apt_get[@]}" update
"${apt_get[@]}" install -y "${packages[@]}"
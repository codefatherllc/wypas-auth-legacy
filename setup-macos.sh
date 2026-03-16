#!/bin/bash
set -e

# Build tools + library deps via Homebrew
brew install cmake ninja pkg-config ccache \
    boost zlib openssl luajit

echo "Ready. Run:"
echo "  cmake --preset macos-debug"
echo "  cmake --build --preset macos-debug"

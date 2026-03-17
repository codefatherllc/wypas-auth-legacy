#!/bin/bash
set -e

# Build tools + library deps via Homebrew
brew install cmake ninja ccache \
    boost zlib openssl

echo "Ready. Run:"
echo "  cmake --preset macos-debug"
echo "  cmake --build --preset macos-debug"

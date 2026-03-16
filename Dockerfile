# ---- Build stage ----
FROM ubuntu:24.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build git curl zip unzip tar pkg-config \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install vcpkg — fetch baseline commit so manifest mode can resolve versions
ENV VCPKG_ROOT=/opt/vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git $VCPKG_ROOT \
    && $VCPKG_ROOT/bootstrap-vcpkg.sh -disableMetrics

WORKDIR /src
COPY vcpkg.json CMakeLists.txt CMakePresets.json ./
COPY src/ src/

# Configure and build
RUN cmake --preset linux-release \
    && cmake --build --preset linux-release -- -j2

# ---- Runtime stage ----
FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 zlib1g libssl3t64 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /server

COPY --from=build /src/build/linux-release/theforgottenloginserver .

EXPOSE 7171

ENTRYPOINT ["./theforgottenloginserver"]

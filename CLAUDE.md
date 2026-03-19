# wypas-auth-legacy — C++ TCP Login Server

## Build

```bash
cmake --preset macos-debug && cmake --build --preset macos-debug     # macOS
cmake --preset linux-release && cmake --build --preset linux-release  # Linux
```

Output: `build/<preset>/theforgottenloginserver`

## Config

`config.json` + `worlds.json` (shared with wypas-auth). `--path=<dir>` CLI arg.

## Purpose

Native client authentication on TCP port 7171. RSA handshake + XTEA encryption. SHA1(salt+password) auth. HTTP login removed — browser login goes through wypas-auth (Go).

## Key Files

`src/protocol/protocollogin.cpp` (login protocol), `src/net/connection.cpp` (TCP + XTEA), `src/database.cpp` (Boost.MySQL), `src/io/worlds.cpp` (world list).

## Dependencies

Boost (headers, charconv, json, thread, mysql), OpenSSL, ZLIB. Via vcpkg.

## CI

`.github/workflows/build.yml` on `main`. Green.

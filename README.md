# wypas-auth-legacy

C++ TCP login server for the Wypas Tibia project. Handles native client authentication on port 7171.

## Overview

Validates account credentials over the Tibia 9.63 TCP login protocol, checks bans, and returns character list + game server addresses. For native desktop clients only.

Web/WASM clients use the Go-based [wypas-auth](https://github.com/codefatherllc/wypas-auth) service instead.

## Features

- **TCP port 7171 only** — HTTP login removed
- **9.63 protocol** — no multi-version support
- **SHA1(salt + password)** authentication
- **RSA handshake** + XTEA encryption
- **IP and account ban enforcement**
- **Connection rate limiting** per IP
- **Boost.MySQL** database driver (MySQL only, no SQLite)
- **JSON world config** (`worlds.json` replaces `servers.xml`)

## Prerequisites

### macOS

```bash
./setup-macos.sh
```

### Linux (via vcpkg)

CMake + vcpkg handles dependencies automatically. Only `ninja-build` needed system-wide:

```bash
sudo apt-get install -y ninja-build
```

## Build

### macOS

```bash
cmake --preset macos-debug
cmake --build --preset macos-debug
```

### Linux

```bash
cmake --preset linux-release
cmake --build --preset linux-release
```

### Docker

```bash
docker build -t auth-legacy .
```

Binary: `theforgottenloginserver`

## Configuration

### config.json

Shared with wypas-auth. Both services read from the same file.

```json
{
  "database": { "dsn": "mysql://wypas:pass@localhost:3306/wypas" },
  "auth": { "encryptionType": "sha1" },
  "legacy": {
    "listenAddr": "0.0.0.0:7171",
    "serverName": "Wypas",
    "motdText": "Welcome to Wypas!",
    "rsa": { "prime1": "...", "prime2": "...", "public": "65537", "modulus": "...", "private": "..." }
  }
}
```

See `config.json.dist` for all options. Use `--path=<dir>` to specify config directory.

### worlds.json

```json
[
  { "id": 0, "name": "Wypas", "address": "127.0.0.1", "ports": [7172] }
]
```

## Login Flow

1. Client connects TCP:7171
2. RSA handshake → extract XTEA key
3. Validate credentials: SHA1(salt + password)
4. Check IP/account bans
5. Return MOTD + character list with game server addresses
6. Client disconnects, connects to game server directly

## CI

GitHub Actions: `.github/workflows/build.yml` on push to `master`.

**Jobs:**
- `linux` — amd64 + arm64 (CMake + vcpkg + sccache)
- `macos-arm64` — Homebrew deps + sccache
- `docker` — Multi-arch GHCR image (push to master only)
- `release` — GitHub Release with tarballs (push to master only)

## Key Files

| File | Purpose |
|------|---------|
| `src/protocol/protocollogin.cpp` | Login protocol: version check, RSA, auth, char list |
| `src/net/connection.cpp` | TCP socket, message framing, XTEA |
| `src/database.cpp` | Boost.MySQL database interface |
| `src/io/worlds.cpp` | Loads worlds.json (Boost.JSON) |
| `src/io/io.cpp` | Account/ban lookups |
| `src/configmanager.cpp` | Loads config.json (Boost.JSON) |
| `src/net/server.cpp` | Boost.Asio acceptor |
| `config.json.dist` | Reference configuration |
| `worlds.json` | Game server directory |

## Related Repos

| Repo | Purpose |
|------|---------|
| `wypas-auth` | Go HTTP login (browser/WASM clients) |
| `wypas-server` | Game server (TCP 7172 + WS 7173) |
| `wypas-proxy` | Docker Compose deployment |

## License

GNU General Public License v3.0

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

## Prerequisites

Ubuntu 24.04:

```bash
sudo apt-get install -y \
  autoconf automake libtool pkg-config \
  libmysqlclient-dev liblua5.1-dev libgmp-dev \
  libxml2-dev libssl-dev zlib1g-dev libsqlite3-dev
```

Boost 1.90 (build from source):

```bash
wget -q https://archives.boost.io/release/1.90.0/source/boost_1_90_0.tar.gz
tar xf boost_1_90_0.tar.gz && cd boost_1_90_0
./bootstrap.sh --with-libraries=system,thread,regex,filesystem,date_time,chrono
sudo ./b2 install -j$(nproc) --prefix=/usr/local && sudo ldconfig
```

## Build

```bash
./autogen.sh
./configure --enable-mysql    # or --enable-sqlite
make -j$(nproc)
```

Binary: `theforgottenloginserver`

## Configuration

### config.lua

```lua
ip = "127.0.0.1"
loginPort = 7171
loginTimeout = 60 * 1000
loginTries = 10
retryTimeout = 5 * 1000

sqlType = "mysql"
sqlHost = "localhost"
sqlPort = 3306
sqlUser = "wypas"
sqlPass = ""
sqlDatabase = "wypas"

encryptionType = "sha1"
serverName = "Wypas"
motdText = "Welcome to Wypas!"
```

### servers.xml

```xml
<servers>
  <server id="0" name="Wypas" address="127.0.0.1" port="7172"/>
</servers>
```

## Login Flow

1. Client connects TCP:7171
2. RSA handshake → extract XTEA key
3. Validate credentials: SHA1(salt + password)
4. Check IP/account bans
5. Return MOTD + character list with game server addresses
6. Client disconnects, connects to game server directly

## CI

GitHub Actions: `.github/workflows/build.yml` on push to `master`. Builds on Ubuntu 24.04. Status: green.

## Key Files

| File | Purpose |
|------|---------|
| `protocollogin.cpp` | Login protocol: version check, RSA, auth, char list |
| `connection.cpp` | TCP socket, message framing, XTEA |
| `database.cpp` | Abstract DB interface (MySQL/SQLite) |
| `gameservers.cpp` | Loads servers.xml |
| `io.cpp` | Account/ban lookups |
| `configmanager.cpp` | Loads config.lua |
| `server.cpp` | Boost.Asio acceptor |
| `config.lua` | Runtime configuration |
| `servers.xml` | Game server directory |

## Related Repos

| Repo | Purpose |
|------|---------|
| `wypas-auth` | Go HTTP login (browser/WASM clients) |
| `wypas-server` | Game server (TCP 7172 + WS 7174) |
| `wypas-proxy` | Docker Compose deployment |

## License

GNU General Public License v3.0

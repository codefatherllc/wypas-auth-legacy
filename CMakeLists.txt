cmake_minimum_required(VERSION 3.16)
project(wypas-auth-legacy VERSION 2.0 LANGUAGES CXX)

if(WIN32)
    message(FATAL_ERROR "Windows is not supported. Use macOS (clang) or Linux (g++).")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Options ---
option(ENABLE_DEBUG "Enable debug build" OFF)
option(ENABLE_ROOT_PERMISSION "Allow running as root" OFF)

# --- Sources ---
set(SOURCES
    # core
    src/otlogin.cpp
    src/otpch.cpp
    src/configmanager.cpp
    src/tasks.cpp
    src/scheduler.cpp
    src/textlogger.cpp
    src/tools.cpp
    src/database.cpp

    # net
    src/net/connection.cpp
    src/net/networkmessage.cpp
    src/net/outputmessage.cpp
    src/net/server.cpp

    # protocol
    src/protocol/protocol.cpp
    src/protocol/protocollogin.cpp

    # io
    src/io/io.cpp
    src/io/worlds.cpp
)

add_executable(theforgottenloginserver ${SOURCES})

# --- Compile definitions ---
target_compile_definitions(theforgottenloginserver PRIVATE
    _THREAD_SAFE
    _REENTRANT
    __USE_MYSQL__
)

if(ENABLE_ROOT_PERMISSION)
    target_compile_definitions(theforgottenloginserver PRIVATE __ROOT_PERMISSION__)
endif()

if(ENABLE_DEBUG)
    target_compile_definitions(theforgottenloginserver PRIVATE
        __DEBUG__ __DEBUG_NET__ __DEBUG_NET_DETAIL__
        __DEBUG_SCHEDULER__ __SQL_QUERY_DEBUG__
    )
    target_compile_options(theforgottenloginserver PRIVATE -g3)
endif()

# --- Compiler flags ---
target_compile_options(theforgottenloginserver PRIVATE
    -Wall -Wextra
    -Wno-strict-aliasing
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-xor-used-as-pow -Wno-overloaded-virtual>
    -fomit-frame-pointer
)

# Suppress sprintf deprecation warnings in legacy TFS files (macOS)
set_source_files_properties(
    src/tools.cpp src/otlogin.cpp src/protocol/protocollogin.cpp
    PROPERTIES COMPILE_OPTIONS "-Wno-deprecated-declarations"
)

# --- Dependencies (vcpkg on Linux, Homebrew on macOS) ---
# Boost.MySQL is header-only — no cmake component needed, just Boost::headers
find_package(Boost REQUIRED COMPONENTS charconv filesystem json thread)
target_link_libraries(theforgottenloginserver PRIVATE
    Boost::headers Boost::charconv Boost::filesystem Boost::json Boost::thread
)

find_package(ZLIB REQUIRED)
target_link_libraries(theforgottenloginserver PRIVATE ZLIB::ZLIB)

find_package(OpenSSL REQUIRED)
target_link_libraries(theforgottenloginserver PRIVATE OpenSSL::Crypto OpenSSL::SSL)

find_package(Threads REQUIRED)
target_link_libraries(theforgottenloginserver PRIVATE Threads::Threads)

# --- Includes ---
target_include_directories(theforgottenloginserver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/net
    ${CMAKE_CURRENT_SOURCE_DIR}/src/protocol
    ${CMAKE_CURRENT_SOURCE_DIR}/src/io
)

# ccache: use -DCMAKE_C_COMPILER_LAUNCHER=ccache in CI.

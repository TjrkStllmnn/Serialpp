# Serial++

## Features
- a simple-to-use wrapper around Windows serial communication
- a simple-to-use wrapper around sockets
- simple OpenSSL support for sockets

## Dependencies
- Windows (for Serial; the Sockets __may__ work under Linux)
- a compiler (on windows it's ususally shipped with Visual Studio, alternatively clang or g++)
- (optional, recommended) CMake (Meta-Buildsystem)
- (optional, recommended) Ninja (Build file generator)

## How to use
The simplest should be to make a CMake project and add this project as subdirectory and link the executable (or library) with it.

    add_subdirectory(Serial++)
    target_link_library(<target> Serial++)


## Notes
For browser-based documentation use Doxygen.

To use `tcp_socket`s (not the server) with ssl you need to `#define USE_OPENSSL` before including the Socket header and also link to OpenSSL. You then have to connect to the server and perform the ssl handshake.

The `wsa_handler` as well as the `openssl_handler` are not intended for general use, as they are helper classes to manage the one time setup for both.

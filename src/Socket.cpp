#include "Socket.hpp"

#include <chrono>
#include <thread>

namespace {
int sock_close(SOCKET& sock) {
  int status{ 0 };
  status = shutdown(sock, SD_BOTH);
  if (status == 0)
    status = closesocket(sock);
  return status;
}
} // namespace

namespace socketio {
wsa_handler::wsa_handler() {
#ifdef _WIN32
  WSADATA wsa_data;
  WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
}

wsa_handler::~wsa_handler() {
#ifdef _WIN32
  WSACleanup();
#endif
}

openssl_handler::openssl_handler() {
#ifdef USE_OPENSSL
  OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS
                       | OPENSSL_INIT_LOAD_CRYPTO_STRINGS,
                   nullptr);

  OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG | OPENSSL_INIT_ADD_ALL_CIPHERS
                          | OPENSSL_INIT_ADD_ALL_DIGESTS,
                      nullptr);
#endif
}

openssl_handler::~openssl_handler() {
#ifdef USE_OPENSSL
  ERR_free_strings();
#endif
}

ssl_exception::ssl_exception(const char* msg) noexcept
    : msg_{ msg } {
}

ssl_exception::ssl_exception(const std::string& msg) noexcept
    : msg_{ msg } {
}

const char* ssl_exception::what() const noexcept {
  return msg_.c_str();
}

socket_exception::socket_exception(const char* msg) noexcept
    : msg_{ msg } {
}

socket_exception::socket_exception(const std::string& msg) noexcept
    : msg_{ msg } {
}

const char* socket_exception::what() const noexcept {
  return msg_.c_str();
}

tcp_socket::tcp_socket(SOCKET s)
    : ept_{} {
  sock = s;
}

std::string tcp_socket::get_ssl_error() {
#ifdef USE_OPENSSL
  return std::string{ ERR_error_string(0, nullptr) };
#else
  return "";
#endif
}

endpoint::endpoint()
    : addr_info{ nullptr }
    , domain{}
    , port{} {
}

endpoint::endpoint(const char* domain, const char* port)
    : endpoint{} {
  this->domain = domain;
  this->port   = static_cast<short>(atoi(port));
  int error    = getaddrinfo(domain, port, nullptr, &addr_info);
  if (error) {
    throw socket_exception{ "Error getting address info: "
                            + std::string{ gai_strerror(error) } };
  }
}

endpoint::endpoint(const std::string& domain, const char* port)
    : endpoint{ domain.c_str(), port } {
}

endpoint::~endpoint() {
  freeaddrinfo(addr_info);
  addr_info = nullptr;
}

endpoint::operator addrinfo*() {
  return addr_info;
}

wsa_handler base_socket::_wsa_handler = wsa_handler{};

base_socket::base_socket()
    : sock{ INVALID_SOCKET } {
}

tcp_socket::tcp_socket()
    : base_socket{}
    , ept_{}
#ifdef USE_OPENSSL
    , ssl{ nullptr }
    , ssl_ctx{ nullptr }
#endif
{
}

tcp_socket::~tcp_socket() {
  close();
}

bool tcp_socket::is_valid() const {
  return sock != INVALID_SOCKET;
}

bool tcp_socket::is_connected() const {
  return sock != INVALID_SOCKET;
}

bool tcp_socket::is_secure() const {
#ifdef USE_OPENSSL
  return ssl != nullptr;
#else
  return false;
#endif
}

void tcp_socket::connect(endpoint ept) {
  ept_ = ept;
  std::string error_string{ "" };
  for (struct addrinfo* cur_addr_info = ept_; cur_addr_info != nullptr;
       cur_addr_info                  = cur_addr_info->ai_next) {
    sock = ::socket(cur_addr_info->ai_family, cur_addr_info->ai_socktype,
                    cur_addr_info->ai_protocol);
    if (!sock) {
      error_string = "Unable to open socket";
      continue;
    }
    if (::connect(sock, cur_addr_info->ai_addr, cur_addr_info->ai_addrlen)) {
      error_string = "Unable to connect";
      sock_close(sock);
      sock = INVALID_SOCKET;
      continue;
    }
    break;
  }
  if (!sock) {
    throw socket_exception{ error_string };
  }
}

void tcp_socket::connect(const char* domain, const char* port) {
  connect({ domain, port });
}

void tcp_socket::close() {
#ifdef USE_OPENSSL
  if (ssl) {
    SSL_shutdown(ssl);
    SSL_free(ssl);
    ssl = nullptr;
  }
  if (ssl_ctx) {
    SSL_CTX_free(ssl_ctx);
    ssl_ctx = nullptr;
  }
#endif
  if (sock) {
    sock_close(sock);
    sock = INVALID_SOCKET;
  }
  ept_ = endpoint{};
}

void tcp_socket::ssl_handshake() {
#ifdef USE_OPENSSL
  static openssl_handler _ssl_handler_life;

  ssl_ctx = SSL_CTX_new(TLS_client_method());
  if (!ssl_ctx) {
    throw ssl_exception{ "Unable to create SSL context: " + get_ssl_error() };
  }
  ssl = SSL_new(ssl_ctx);
  if (!ssl) {
    SSL_CTX_free(ssl_ctx);
    ssl_ctx = nullptr;
    throw ssl_exception{ "Unable to create SSL handle: " + get_ssl_error() };
  }

  // pair ssl with socket
  if (!SSL_set_fd(ssl, sock)) {
    SSL_free(ssl);
    SSL_CTX_free(ssl_ctx);
    ssl     = nullptr;
    ssl_ctx = nullptr;
    throw ssl_exception{ "Unable to associate SSL and plain socket: "
                         + get_ssl_error() };
  }

  // ssl handshake
  for (int error = SSL_connect(ssl); error != 1; error = SSL_connect(ssl)) {
    switch (SSL_get_error(ssl, error)) {
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        break;
      case SSL_ERROR_SSL:
      default:
        SSL_free(ssl);
        SSL_CTX_free(ssl_ctx);
        ssl     = nullptr;
        ssl_ctx = nullptr;
        throw ssl_exception{ "Error in SSL handshake: " + get_ssl_error() };
        break;
    }
  }
#else
  throw ssl_exception{
    "To use the ssl_handshake function you need to #define USE_OPENSSL"
  };
#endif
}

int tcp_socket::write(const byte* buffer, size_t size, int flags) {
  if (is_secure()) {
    return swrite(buffer, size);
  } else {
    return uwrite(buffer, size, flags);
  }
}

int tcp_socket::write(const byte b) {
  return write(&b, 1);
}

int tcp_socket::write(const std::string& str) {
  return write((byte*) str.c_str(), str.length());
}

int tcp_socket::writeLine(const std::string& str) {
  return write((byte*) (str + "\n").c_str(), str.length() + 1);
}

int tcp_socket::read(byte* buffer, size_t size, int flags) {
  if (is_secure()) {
    return sread(buffer, size);
  } else {
    return uread(buffer, size, flags);
  }
}

byte tcp_socket::read() {
  byte b{};
  read(&b, 1);
  return b;
}

std::string tcp_socket::readLine() {
  std::string s{};
  char        c{};
  while ((c = read()) != '\n') {
    s.push_back(c);
  }
  return s;
}

int tcp_socket::uwrite(const byte* buffer, size_t size, int flags) {
  return ::send(sock, (char*) buffer, size, flags);
}

int tcp_socket::uread(byte* buffer, size_t size, int flags) {
  return ::recv(sock, (char*) buffer, size, flags);
}

int tcp_socket::swrite(const byte* buffer, size_t size) {
#ifdef USE_OPENSSL
  int written = SSL_write(ssl, (void*) buffer, size);
  if (written > 0) {
    return written;
  } else {
    switch (SSL_get_error(ssl, written)) {
      case SSL_ERROR_ZERO_RETURN: // The socket has been closed on the other end
        close();
        throw sock_exception{ "The socket disconnected" };
        break;
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        break;
      default:
        throw ssl_exception{ "Error sending socket: " + get_ssl_error() };
        break;
    }
  }
#endif
  return 0;
}

int tcp_socket::sread(byte* buffer, size_t size) {
#ifdef USE_OPENSSL
  size_t read_size = SSL_read(ssl, (void*) buffer, size);
  if (read_size > 0) {
    return read_size;
  } else {
    switch (SSL_get_error(ssl, read_size)) {
      case SSL_ERROR_ZERO_RETURN:
        disconnect();
        return 0;
        break;
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        return 0;
        break;
      default:
        throw ssl_exception("Error reading socket: " + get_ssl_error());
        break;
    }
  }
#endif
  return -1;
}

tcp_server_socket::tcp_server_socket(const char* port) {
  endpoint e{ "localhost", port };
  sock = ::socket(((addrinfo*) e)->ai_family, ((addrinfo*) e)->ai_socktype,
                  ((addrinfo*) e)->ai_protocol);
  if (sock == INVALID_SOCKET) {
    throw socket_exception{ "could not create socket" };
  }
  if (::bind(sock, ((addrinfo*) e)->ai_addr,
             (int) ((addrinfo*) e)->ai_addrlen)) {
    throw socket_exception{ "could not bind to port" };
  }
  if (::listen(sock, SOMAXCONN) == SOCKET_ERROR) {
    throw socket_exception{ "could not listen" };
  }
}

tcp_server_socket::~tcp_server_socket() {
  close();
}

[[nodiscard]] tcp_socket tcp_server_socket::accept() {
  SOCKET s{ INVALID_SOCKET };
  s = ::accept(sock, NULL, NULL);
  if (s == INVALID_SOCKET) {
    throw socket_exception{ "failed to accept connection" };
  }
  return { s };
}

void tcp_server_socket::close() {
  ::sock_close(sock);
}
} // namespace socketio

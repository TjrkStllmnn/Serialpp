#pragma once

#ifdef _WIN32 // Windows
#include <ws2tcpip.h>
#include <winsock2.h>
#define MSG_NOSIGNAL 0
#else // Linux + Mac

#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SOCKET_ERROR   -1
#define INVALID_SOCKET -1

typedef int         SOCKET;
typedef sockaddr    SOCKADDR;
typedef sockaddr_in SOCKADDR_IN;

#define closesocket close
#define SD_BOTH     SHUT_RDWR

#endif

#ifdef USE_OPENSSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/types.h>
#endif

#include <exception>
#include <memory>
#include <string>

#include <string.h>

namespace socketio {
typedef unsigned char byte;

class wsa_handler {
public:
  wsa_handler();
  ~wsa_handler();
};

class openssl_handler {
public:
  openssl_handler();
  ~openssl_handler();
};

class socket_exception : std::exception {
protected:
  std::string msg_;

public:
  explicit socket_exception(const char* msg) noexcept;
  explicit socket_exception(const std::string& msg) noexcept;
  virtual ~socket_exception() noexcept = default;
  virtual const char* what() const noexcept;
};

class ssl_exception : std::exception {
protected:
  std::string msg_;

public:
  explicit ssl_exception(const char* msg) noexcept;
  explicit ssl_exception(const std::string& msg) noexcept;
  virtual ~ssl_exception() noexcept = default;
  virtual const char* what() const noexcept;
};

/**
 * @brief socket endpoint
 *
 */
class endpoint {
  friend class base_socket;
  std::string domain;
  short       port;

  addrinfo* addr_info;

public:
  endpoint();
  endpoint(const char* domain, const char* port);
  endpoint(const std::string& domain, const char* port);

  ~endpoint();

  operator addrinfo*();

  // friend std::ostream& operator<<(std::ostream& out, const endpoint& ep);
};

/**
 * @brief base class for sockets
 *
 */
class base_socket {
  static wsa_handler _wsa_handler;

protected:
  SOCKET sock;

  base_socket();
  virtual ~base_socket() = default;
};

/**
 * @brief basic tcp socket
 *
 */
class tcp_socket : base_socket {
  friend class tcp_server_socket;

private:
  endpoint ept_;

#ifdef USE_OPENSSL
  SSL*     ssl;
  SSL_CTX* ssl_ctx;
#endif

  tcp_socket(SOCKET s);

  /**
   * @brief Get the ssl error object
   *
   * @return std::string
   */
  std::string get_ssl_error();

  /**
   * @brief unsafe write(no ssl)
   *
   * @param buffer
   * @param size
   * @param flags
   * @return int
   */
  int uwrite(const byte* buffer, size_t size, int flags = 0);
  /**
   * @brief unsafe read(no ssl)
   *
   * @param buffer
   * @param size
   * @param flags
   * @return int
   */
  int uread(byte* buffer, size_t size, int flags = 0);

  /**
   * @brief safe write(ssl)
   *
   * @param buffer
   * @param size
   * @return int
   */
  int swrite(const byte* buffer, size_t size);
  /**
   * @brief safe read(ssl)
   *
   * @param buffer
   * @param size
   * @return int
   */
  int sread(byte* buffer, size_t size);

public:
  tcp_socket();
  ~tcp_socket();

  /**
   * @brief is the socket valid
   *
   * @return true
   * @return false
   */
  bool is_valid() const;

  /**
   * @brief is the socket connected to the endpoint
   *
   * @return true
   * @return false
   */
  bool is_connected() const;
  /**
   * @brief is connected using ssl
   *
   * @return true
   * @return false
   */
  bool is_secure() const;

  /**
   * @brief connects to endpoint
   *
   * @param ept
   */
  void connect(endpoint ept);
  /**
   * @brief creates endpoint out of domain and port and connects to it
   *
   * @param domain
   * @param port
   */
  void connect(const char* domain, const char* port);

  /**
   * @brief closes the connection
   *
   */
  void close();

  /**
   * @brief performs ssl handshake
   *
   */
  void ssl_handshake();

  /**
   * @brief writes byte array
   * @note writes save if ssl_handshake has been performed else writes unsafe
   * @param buffer byte array
   * @param size size of the buffer
   * @param flags
   * @return int bytes written
   */
  int write(const byte* buffer, size_t size, int flags = 0);
  /**
   * @brief writes single byte (using write())
   *
   * @param b
   * @return int bytes written
   */
  int write(const byte b);
  /**
   * @brief writes string (using write())
   *
   * @param str
   * @return int bytes written
   */
  int write(const std::string& str);
  /**
   * @brief writes string and appends a newline (using write())
   *
   * @param str
   * @return int bytes written
   */
  int writeLine(const std::string& str);

  /**
   * @brief reads byte array
   * @note reads save if ssl_handshake has been performed else reads unsafe
   * @param buffer byte array
   * @param size size of the buffer
   * @param flags
   * @return int bytes read
   */
  int read(byte* buffer, size_t size, int flags = 0);
  /**
   * @brief reads a single byte
   *
   * @return byte
   */
  byte read();
  /**
   * @brief reads a string up to a newline
   *
   * @return std::string
   */
  std::string readLine();
};

/**
 * @brief basic tcp server
 *
 */
class tcp_server_socket : base_socket {
private:
public:
  tcp_server_socket(const char* port);
  ~tcp_server_socket();

  /**
   * @brief accepts a tcp connection
   *
   * @return tcp_socket
   */
  [[nodiscard]] tcp_socket accept();
  /**
   * @brief stops listening
   *
   */
  void close();
};
} // namespace socketio

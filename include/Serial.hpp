#pragma once

#include <exception>
#include <string>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace serialio {
typedef unsigned char byte;

enum Baud : unsigned int {
  BD_110    = CBR_110,
  BD_300    = CBR_300,
  BD_600    = CBR_600,
  BD_1200   = CBR_1200,
  BD_2400   = CBR_2400,
  BD_4800   = CBR_4800,
  BD_9600   = CBR_9600,
  BD_14400  = CBR_14400,
  BD_19200  = CBR_19200,
  BD_38400  = CBR_38400,
  BD_56000  = CBR_56000,
  BD_57600  = CBR_57600,
  BD_115200 = CBR_115200,
  BD_128000 = CBR_128000,
  BD_256000 = CBR_256000
};

enum StopBits : unsigned char {
  BITS_1  = ONESTOPBIT,
  BITS_10 = STOPBITS_10,
  BITS_15 = STOPBITS_15,
  BITS_20 = STOPBITS_20
};

enum Parity : unsigned char {
  NO_PRT    = NOPARITY,
  EVEN_PRT  = EVENPARITY,
  ODD_PRT   = ODDPARITY,
  MARK_PRT  = MARKPARITY,
  SPACE_PRT = SPACEPARITY
};

enum Event : unsigned int {
  BREAK         = EV_BREAK,
  CTS_CHANGED   = EV_CTS,
  DSR_CHANGED   = EV_DSR,
  ERR           = EV_ERR,
  RING          = EV_RING,
  RLSD_CHANGED  = EV_RLSD,
  CHAR_REVEIVED = EV_RXCHAR,
  CHAR_FLAG     = EV_RXFLAG,
  OUTPUT_EMPTY  = EV_TXEMPTY
};

class serial_exception : public std::exception {
private:
  const char* info_;
  const char* msg_;

public:
  explicit serial_exception(DWORD info) noexcept;
  explicit serial_exception(const char* msg) noexcept;
  virtual ~serial_exception() noexcept = default;
  virtual const char* what() const noexcept;

  const char* getInfo() noexcept;
};

class serial_port {
  friend class serial;

private:
  std::string name;

public:
  serial_port(const char* name)
      : serial_port{ std::string{ name } } {
  }

  serial_port(std::string name) {
    this->name = name;
  }

  serial_port(int num) {
    this->name = "COM" + std::to_string(num);
  }

  friend std::ostream& operator<<(std::ostream& out, const serial_port& port) {
    return out << port.name;
  }
};

/**
 * @brief An object of the Serial class encapsulates a serial-interface and
 * therefor a serial port.
 *
 */
class serial {
private:
  HANDLE            serialHandle;
  const serial_port port_;
  const Baud        baud_;
  const int         byteSize_;
  const StopBits    stopBits_;
  const Parity      parity_;
  bool              open_;

public:
  /**
   * @brief Constructor
   *
   * @param port the com port
   * @param baud bytes per second
   * @param byteSize size of the byte
   * @param stopBits number of stop bits
   * @param parity type of parity
   */
  serial(const serial_port& port, const Baud baud, const int byteSize,
         const StopBits stopBits, const Parity parity);

  ~serial();

  /**
   * @brief opens connection
   *
   * @return true if successful
   */
  bool open();

  /**
   * @brief closes serial connection
   *
   * @return true if successful
   */
  bool close();

  /**
   * @brief is connection currently open
   */
  bool isOpen() const;

  /**
   * @brief sets DTR line
   *
   * @param dtr
   */
  void setDTR(bool dtr);

  /**
   * @brief sets RTS line
   *
   * @param rts
   */
  void setRTS(bool rts);

  /**
   * @brief is DSR high
   */
  bool isDSR() const;

  /**
   * @brief is CTS high
   */
  bool isCTS() const;

  /**
   * @brief Blocking. \n waits for event to occur. \n Experimental
   *
   * @param event event to wait for
   */
  [[deprecated("Not yet implemented")]] void waitFor(Event event) const;

  /**
   * @brief reads bytes into buffer
   *
   * @param[out] buffer buffer to write into
   * @param bytes number of bytes to read
   * @return unsigned long number of bytes read
   */
  unsigned long read(byte buffer[], unsigned int bytes);

  /**
   * @brief read 1 byte from input buffer
   *
   * @return byte read byte
   */
  byte read();

  /**
   * @brief Blocking. read until first occurrence of \\n
   *
   * @return string read string without last \\n
   */
  std::string readLine();

  /**
   * @brief write bytes from buffer
   *
   * @param[in] buffer buffer to write from
   * @param bytes number of bytes to write
   * @return true if write was successful
   */
  bool write(byte buffer[], unsigned int bytes);

  /**
   * @brief writes byte
   *
   * @param b byte to write
   */
  void write(byte b);

  /**
   * @brief writes string
   *
   * @param s string to write
   */
  void write(const std::string& s);

  /**
   * @brief writes string and appends a \\n
   *
   * @param s string to write
   */
  void writeLine(const std::string& s);

  /**
   * @brief returns bytes in input buffer. (clears comm errors)
   *
   * @return unsigned int bytes available
   */
  unsigned int dataAvailable();

  /**
   * @brief returns available ports for connection
   *
   * @return std::vector<Port> Ports that are available
   */
  static std::vector<serial_port> getAvailablePorts();

  /*
  void operator<<(const char *c);
  void operator<<(const std::string &s);
  void operator<<(std::ostream &o);

  void operator>>(byte &b);
  void operator>>(char &c);
  void operator>>(std::string &s);
  void operator>>(std::iostream &io);
  */
};
} // namespace serialio

#include "Serial.hpp"

namespace serialio {
serial_exception::serial_exception(DWORD info) noexcept
    : msg_{ nullptr } {
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                    | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, info, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &info_, 0, nullptr);
}

serial_exception::serial_exception(const char* msg) noexcept
    : info_{ nullptr } {
  msg_ = msg;
}

const char* serial_exception::getInfo() noexcept {
  return info_;
}

const char* serial_exception::what() const noexcept {
  if (msg_) {
    return msg_;
  }
  if (info_) {
    return "Serial exception: see getInfo() for more";
  }
  return "Serial exception";
}

serial::serial(const serial_port& port, const Baud baud, const int byteSize,
               const StopBits stopBits, const Parity parity)
    : serialHandle{ nullptr }
    , port_{ port }
    , baud_{ baud }
    , byteSize_{ byteSize }
    , stopBits_{ stopBits }
    , parity_{ parity }
    , open_{ false } {
}

serial::~serial() {
  if (open_)
    close();
}

bool serial::open() {
  serialHandle = CreateFile((R"(\\.\)" + port_.name).c_str(),
                            GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (serialHandle == INVALID_HANDLE_VALUE)
    if (GetLastError() == ERROR_FILE_NOT_FOUND)
      throw serial_exception{ ("Port not found: " + port_.name).c_str() };

  DCB serialParams;
  SecureZeroMemory(&serialParams, sizeof(DCB));
  serialParams.DCBlength = sizeof(DCB);

  GetCommState(serialHandle, &serialParams);
  serialParams.BaudRate    = baud_;
  serialParams.ByteSize    = byteSize_;
  serialParams.StopBits    = stopBits_;
  serialParams.Parity      = parity_;
  serialParams.fRtsControl = RTS_CONTROL_DISABLE;
  serialParams.fDtrControl = DTR_CONTROL_DISABLE;
  if (!SetCommState(serialHandle, &serialParams))
    throw serial_exception{ GetLastError() };

  COMMTIMEOUTS timeout                = { 0 };
  timeout.ReadIntervalTimeout         = 50;
  timeout.ReadTotalTimeoutConstant    = 50;
  timeout.ReadTotalTimeoutMultiplier  = 50;
  timeout.WriteTotalTimeoutConstant   = 50;
  timeout.WriteTotalTimeoutMultiplier = 10;
  if (!SetCommTimeouts(serialHandle, &timeout))
    throw serial_exception{ GetLastError() };

  if (!GetCommState(serialHandle, &serialParams))
    throw serial_exception{ GetLastError() };

  open_ = true;
  return true;
}

bool serial::close() {
  setDTR(false);
  setRTS(false);
  FindClose(serialHandle);
  if (!CloseHandle(serialHandle))
    throw serial_exception{ GetLastError() };
  open_ = false;
  return true;
}

bool serial::isOpen() const {
  return open_;
}

void serial::setDTR(bool dtr) {
  EscapeCommFunction(serialHandle, dtr ? SETDTR : CLRDTR);
}

void serial::setRTS(bool rts) {
  EscapeCommFunction(serialHandle, rts ? SETRTS : CLRRTS);
}

bool serial::isDSR() const {
  DWORD modemStat;
  GetCommModemStatus(serialHandle, &modemStat);
  return modemStat & MS_DSR_ON;
}

bool serial::isCTS() const {
  DWORD modemStat;
  GetCommModemStatus(serialHandle, &modemStat);
  return modemStat & MS_CTS_ON;
}

void serial::waitFor(Event event) const {
  WaitCommEvent(this->serialHandle, (LPDWORD) &event, NULL);
}

unsigned long serial::read(byte buffer[], unsigned int bytes) {
  OVERLAPPED osReader{ 0 };
  DWORD      read{ 0 };
  bool       waitingOnRead{ false };

  osReader.hEvent = CreateEvent(nullptr, true, false, nullptr);
  if (osReader.hEvent == NULL)
    return 0;
  if (!waitingOnRead) {
    if (!ReadFile(this->serialHandle, buffer, bytes, &read, &osReader)) {
      if (GetLastError() != ERROR_IO_PENDING) {
      } else {
        waitingOnRead = true;
      }
    } else {
      CloseHandle(osReader.hEvent);
      return read;
    }
  }
  CloseHandle(osReader.hEvent);
  return read;
}

byte serial::read() {
  byte buffer[1];
  read(buffer, 1);
  return buffer[0];
}

std::string serial::readLine() {
  std::string out{};
  byte        b{};
  while (b != '\n') {
    b = read();
    if (b > 31 && b < 127)
      out.push_back(b);
  }
  return out;
}

bool serial::write(byte buffer[], unsigned int bytes) {
  OVERLAPPED osWrite{ 0 };
  DWORD      written{ 0 };
  bool       successful{ false };

  osWrite.hEvent = CreateEvent(nullptr, true, false, nullptr);
  if (osWrite.hEvent == NULL) {
    return false;
  }
  if (!WriteFile(this->serialHandle, buffer, bytes, &written, &osWrite)) {
    if (GetLastError() != ERROR_IO_PENDING) {
      successful = false;
    } else {
      switch (WaitForSingleObject(osWrite.hEvent, INFINITE)) {
        case WAIT_OBJECT_0:
          if (!GetOverlappedResult(this->serialHandle, &osWrite, &written,
                                   false)) {
            successful = false;
          } else {
            successful = true;
          }
          break;
        default:
          successful = false;
          break;
      }
    }
  } else {
    successful = true;
  }
  CloseHandle(osWrite.hEvent);
  return successful;
}

void serial::write(byte b) {
  write(&b, 1);
}

void serial::write(const std::string& s) {
  write((byte*) s.c_str(), s.length());
}

void serial::writeLine(const std::string& s) {
  write(s + '\n');
}

unsigned int serial::dataAvailable() {
  COMSTAT comStat;
  ClearCommError(this->serialHandle, nullptr, &comStat);
  return comStat.cbInQue;
}

std::vector<serial_port> serial::getAvailablePorts() {
  std::vector<serial_port> ports{};

  char portPath[1000];
  for (int i = 0; i < 255; ++i) {
    if (QueryDosDevice(("COM" + std::to_string(i)).c_str(), portPath, 5000)) {
      ports.push_back(i);
    }
  }
  return ports;
}

/*
void serial::operator<<(const char *c) { this->write(c); }

void serial::operator<<(const std::string &s) { this->write(s); }

void serial::operator<<(std::ostream &o) { this->write((char *)o.rdbuf()); }

void serial::operator>>(byte &b) {
  if (this->dataAvailable())
    b = this->read();
  else
    b = 0;
}

void serial::operator>>(char &c) {
  if (this->dataAvailable())
    c = this->read();
  else
    c = 0;
}

void serial::operator>>(std::string &s) {
  while (this->dataAvailable()) {
    s.append(std::to_string(this->read()));
  }
}

void serial::operator>>(std::iostream &io) {
  while (this->dataAvailable()) {
    io << this->read();
  }
}
*/
} // namespace serialio

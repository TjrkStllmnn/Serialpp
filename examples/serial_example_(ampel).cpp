#include "Serial.hpp"

#include <iostream>

int main(int argc, char** argv) {
  using namespace serialio;
  serial s{ 1, Baud::BD_9600, 8, StopBits::BITS_1, Parity::NO_PRT };
  s.open();
  while (!s.isOpen())
    ;
  s.setDTR(true);
  s.setRTS(true);
  std::cout << "DSR: " << (s.isDSR() ? "true" : "false")
            << "\tCTS: " << (s.isCTS() ? "true" : "false") << "\n";
  s.write(6); // ACK
  s.write("AEIM");
  s.close();
}

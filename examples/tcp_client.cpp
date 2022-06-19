#include "Socket.hpp"

#include <iostream>

int main(int argc, char** argv) {
  socketio::tcp_socket sock{};
  sock.connect("localhost", "1234");
  while (!sock.is_connected())
    ;
  std::cout << sock.readLine() << "\n";
  sock.writeLine("BAR");
  sock.close();
}

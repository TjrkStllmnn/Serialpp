#include "Socket.hpp"

#include <iostream>

int main(int argc, char** argv) {
  socketio::tcp_server_socket server{ "1234" };
  std::cout << "listening\n";
  while (true) {
    auto sock = server.accept();
    std::cout << "accepted socket\n";
    sock.write("FOO\n");
    auto t = sock.readLine();
    std::cout << t << "\n";
  }
  server.close();
}

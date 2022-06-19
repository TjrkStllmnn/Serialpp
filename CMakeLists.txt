cmake_minimum_required(VERSION 3.23.0)
project(
  Serial++
  LANGUAGES CXX
  VERSION 1.1.1
)
set(CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME}
  "include/Serial.hpp"
  "src/Serial.cpp"

  "include/Socket.hpp"
  "src/Socket.cpp"
)

target_include_directories(${PROJECT_NAME}
  PUBLIC "include"
)

if(WIN32)
  target_link_libraries(${PROJECT_NAME} ws2_32)
endif()

# add_subdirectory(examples)
# target_link_libraries(sea ${PROJECT_NAME})
# target_link_libraries(client ${PROJECT_NAME})
# target_link_libraries(server ${PROJECT_NAME})

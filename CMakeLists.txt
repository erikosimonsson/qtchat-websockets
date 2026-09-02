cmake_minimum_required(VERSION 3.21)

project(QtChatWebSockets VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core WebSockets)

qt_add_executable(chat-server
    server/main.cpp
)

target_link_libraries(chat-server PRIVATE
    Qt6::Core
    Qt6::WebSockets
)
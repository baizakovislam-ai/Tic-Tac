cmake_minimum_required(VERSION 3.20)
project(TicTacToeWeb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(tic_tac_server
    src/ai.cpp
    src/classic_game.cpp
    src/game_utils.cpp
    src/main.cpp
    src/multi_game.cpp
    src/session_manager.cpp
    src/static_files.cpp
    src/ultimate_game.cpp
)

target_include_directories(tic_tac_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Crow/include
)

target_compile_definitions(tic_tac_server PRIVATE
    ASIO_STANDALONE
    CROW_DISABLE_COMPRESSION
    _WIN32_WINNT=0x0601
)

if (WIN32)
    target_link_libraries(tic_tac_server PRIVATE ws2_32)
endif()

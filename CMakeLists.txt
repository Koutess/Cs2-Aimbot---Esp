# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(cs2_external LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cs2_external WIN32
    src/main.cpp
)

target_include_directories(cs2_external PRIVATE src)
target_link_libraries(cs2_external PRIVATE user32 gdi32)

if(MSVC)
    target_compile_options(cs2_external PRIVATE /O2 /W3 /DUNICODE /D_UNICODE)
endif()

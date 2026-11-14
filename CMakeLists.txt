cmake_minimum_required(VERSION 3.16)
project(FRIDGE VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive- /EHsc)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# The repository currently contains multiple incompatible generations of the
# C++ library sources and public headers. The shipped CLI implementation in
# src/main.cpp is self-contained, so build it directly instead of combining
# those incompatible sources into fridge_core.
set(FRIDGE_EXE_SOURCES src/main.cpp)
if(WIN32)
    list(APPEND FRIDGE_EXE_SOURCES resources/fridge.rc)
endif()

add_executable(fridge ${FRIDGE_EXE_SOURCES})
set_target_properties(fridge PROPERTIES OUTPUT_NAME "fridge")

install(TARGETS fridge DESTINATION bin)
install(DIRECTORY stdlib/ DESTINATION stdlib OPTIONAL)

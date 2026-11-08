cmake_minimum_required(VERSION 3.20)
project(FRIDGE VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive- /EHsc)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(FRIDGE_SOURCES
    src/main.cpp
    src/core/lexer.cpp
    src/core/parser.cpp
    src/runtime/interpreter.cpp
    src/segments/stdlib_segments.cpp
)

# Embed resources/fridge.ico into fridge.exe on Windows
if(WIN32)
    list(APPEND FRIDGE_SOURCES resources/fridge.rc)
endif()

add_executable(fridge ${FRIDGE_SOURCES})

target_include_directories(fridge PRIVATE include)
set_target_properties(fridge PROPERTIES OUTPUT_NAME "fridge")

install(TARGETS fridge DESTINATION bin)
install(FILES resources/fridge.ico DESTINATION resources)
install(FILES packaging/windows/register_frg_icon.bat DESTINATION bin)
install(DIRECTORY stdlib/ DESTINATION stdlib OPTIONAL)

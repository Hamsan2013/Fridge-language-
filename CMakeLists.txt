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

# Keep the core source list explicit: the repository also contains older,
# duplicate source files at the root of src/ which must not be compiled.
set(FRIDGE_SOURCES
    src/core/diagnostic.cpp
    src/core/lexer.cpp
    src/core/parser.cpp
    src/runtime/value.cpp
    src/runtime/environment.cpp
    src/runtime/interpreter.cpp
    src/package/uzip.cpp
    src/segments/backend.cpp
    src/segments/stdlib.cpp
)

# main.cpp belongs to the CLI executable, not the core library.
list(FILTER FRIDGE_SOURCES EXCLUDE REGEX "(^|[/\\\\])main\\.cpp$")

add_library(fridge_core STATIC ${FRIDGE_SOURCES})
target_include_directories(fridge_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

set(FRIDGE_EXE_SOURCES src/main.cpp)
if(WIN32)
    list(APPEND FRIDGE_EXE_SOURCES resources/fridge.rc)
endif()

add_executable(fridge ${FRIDGE_EXE_SOURCES})
target_link_libraries(fridge PRIVATE fridge_core)
set_target_properties(fridge PROPERTIES OUTPUT_NAME "fridge")

install(TARGETS fridge DESTINATION bin)
install(DIRECTORY stdlib/ DESTINATION stdlib OPTIONAL)

enable_testing()
add_subdirectory(tests)

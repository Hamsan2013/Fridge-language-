cmake_minimum_required(VERSION 3.16)
project(FRIDGE VERSION 0.5.2 LANGUAGES CXX)

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
    src/updater.cpp
)

add_executable(fridge \${FRIDGE_SOURCES})
target_include_directories(fridge PRIVATE \${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(fridge PRIVATE FRIDGE_VERSION="\${PROJECT_VERSION}")
set_target_properties(fridge PROPERTIES OUTPUT_NAME "fridge")

install(TARGETS fridge RUNTIME DESTINATION bin)
install(DIRECTORY stdlib/ DESTINATION stdlib OPTIONAL)

include(CTest)
if(BUILD_TESTING)
    add_executable(fridge_test \${FRIDGE_SOURCES})
    target_include_directories(fridge_test PRIVATE \${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(fridge_test PRIVATE
        FRIDGE_VERSION="\${PROJECT_VERSION}"
        FRIDGE_TESTING=1
    )
    add_test(
        NAME fridge_smoke
        COMMAND \${CMAKE_COMMAND}
            -DFRIDGE_TEST_EXECUTABLE=$<TARGET_FILE:fridge_test>
            -DTEST_WORK_DIR=\${CMAKE_CURRENT_BINARY_DIR}/fridge-test-work
            -P \${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke_test.cmake
    )
endif()

cmake_minimum_required(VERSION 3.10)
project(tests)

add_compile_definitions(BUILD_UNITTEST)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create library
add_library(tests STATIC
    mocks/logger.cpp
    ../main/PropertyParser.cpp
)

# Include directories
target_include_directories(tests PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/../main
)

# GoogleTest setup
enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
message(STATUS "GTest found at: ${GTEST_INCLUDE_DIRS}")

# Create test executable
add_executable(unit-test 
    PropertyParserTests.cpp
    LoggerTests.cpp
    TaskWIFITests.cpp
)

target_link_libraries(unit-test tests GTest::gtest_main)

# Add test
add_test(NAME unit-test COMMAND unit-test)

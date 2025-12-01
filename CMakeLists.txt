cmake_minimum_required(VERSION 3.10)
project(Tcc LANGUAGES CXX)

#Find all source files in src/ EXCEPT for main.cpp
file(GLOB_RECURSE LIB_SOURCES "src/*.cpp")
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(EmulatorLibs STATIC ${LIB_SOURCES})

target_include_directories(EmulatorLibs PUBLIC ${PROJECT_SOURCE_DIR}/include)

add_executable(Tcc src/main.cpp)
target_link_libraries(Tcc PRIVATE EmulatorLibs)

# Tests (optional - only if GTest is found)
find_package(GTest QUIET)
if(GTest_FOUND)
    enable_testing()
    
    # Von Neumann Emulator Tests
    add_executable(emulator_tests tests/von_neumann/emulator_tests.cpp)
    target_link_libraries(emulator_tests
        PRIVATE
            EmulatorLibs
            GTest::gtest
            GTest::gtest_main
    )
    target_include_directories(emulator_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
    add_test(NAME EmulatorTests COMMAND emulator_tests)
    
    # Dataflow Emulator Tests
    add_executable(dataflow_tests tests/dataflow/dataflow_tests.cpp)
    target_link_libraries(dataflow_tests
        PRIVATE
            EmulatorLibs
            GTest::gtest
            GTest::gtest_main
    )
    target_include_directories(dataflow_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
    add_test(NAME DataflowTests COMMAND dataflow_tests)
endif()
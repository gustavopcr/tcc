cmake_minimum_required(VERSION 3.10)
project(Tcc LANGUAGES CXX)

#Find all source files in src/ EXCEPT for main.cpp
file(GLOB_RECURSE LIB_SOURCES "src/*.cpp")
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(EmulatorLibs STATIC ${LIB_SOURCES})

target_include_directories(EmulatorLibs PUBLIC ${PROJECT_SOURCE_DIR}/include)

add_executable(Tcc src/main.cpp)
target_link_libraries(Tcc PRIVATE EmulatorLibs)
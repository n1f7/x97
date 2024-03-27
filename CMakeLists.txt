cmake_minimum_required(VERSION 3.28)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(x97 VERSION 0.0 LANGUAGES CXX)

set(Boost_USE_MULTITHREAD ON)
find_package(Boost REQUIRED COMPONENTS  system)
find_package(Catch2 3 REQUIRED)
include(Catch)

add_library(libX97 INTERFACE)
target_sources(libX97 INTERFACE
	FILE_SET HEADERS
	BASE_DIRS inc/
	FILES
	X97.h
)

add_executable(X97 src/X97.cpp)
target_link_libraries(X97 PRIVATE libX97 Boost::system)

add_executable(tests tests/main.cpp)
target_link_libraries(tests PRIVATE libX97 Catch2::Catch2WithMain)

catch_discover_tests(tests)

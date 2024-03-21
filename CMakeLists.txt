cmake_minimum_required(VERSION 3.28)
set(CMAKE_CXX_EXTENSIONS OFF)
project(x97 VERSION 0.0 LANGUAGES CXX)

set(Boost_USE_MULTITHREAD ON)

find_package(Boost REQUIRED COMPONENTS  system thread)

add_library(x97 INTERFACE)
target_sources(x97 INTERFACE
	FILE_SET HEADERS
	BASE_DIRS inc/
	FILES
	x97.h
)
target_compile_features(x97 INTERFACE cxx_std_23)

add_executable(gX97 src/main.cpp)
target_link_libraries(gX97 PRIVATE x97 Boost::system)


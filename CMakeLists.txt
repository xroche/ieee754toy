cmake_minimum_required(VERSION 3.12)
project(ieee754toy VERSION 1.0.1 DESCRIPTION "IEEE754 constexpr parser toy")

set(CMAKE_CXX_COMPILER "clang++-10")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++ -lc++abi")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

link_libraries(-lc++abi)
set(CMAKE_CXX_FLAGS "-std=c++20 -stdlib=libc++ -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g3")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")

include_directories(include)

add_executable(ieee754toy main.cpp)
target_link_libraries(ieee754toy ${CMAKE_DL_LIBS})

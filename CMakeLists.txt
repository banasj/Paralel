cmake_minimum_required(VERSION 3.16)
project(ParPSys_T5_TBB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find TBB
find_package(TBB REQUIRED)

add_subdirectory(ulohy/1-povinna)
add_subdirectory(ulohy/2-volitelna)
add_subdirectory(ulohy/3-volitelna)

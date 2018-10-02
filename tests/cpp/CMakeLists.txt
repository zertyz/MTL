cmake_minimum_required (VERSION 2.6)

project (CppUtilsSpikes)

set(CMAKE_CXX_STANDARD 17)

#set(CMAKE_CXX_FLAGS -g)
set(CMAKE_CXX_FLAGS -O3)

add_executable(CppUtilsSpikes CppUtilsSpikes.cpp ../../cpp/TimeMeasurements.h ../../cpp/TimeMeasurements.cpp)

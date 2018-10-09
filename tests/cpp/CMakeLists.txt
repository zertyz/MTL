# test this CMakeLists.txt in a standalone fashion as:
# 	TYPE="Release"; rm -fr $TYPE; mkdir $TYPE; cd $TYPE; cmake -G Ninja -DCMAKE_BUILD_TYPE=$TYPE ..; cmake --build .; ctest; cd ..; ls -l "$TYPE/CppUtilsSpikes"
# The rules here should be used when importing this library into other projects
cmake_minimum_required (VERSION 3.9)

project (CppUtilsSpikes VERSION 2018.10.09)

set(CMAKE_CXX_STANDARD 17)


if (NOT TARGET CppUtils)
	find_package(CppUtils REQUIRED)       # find_package is only needed if this CMakeLists.txt is called standalone (in opposition to being included by the uplevel CMakeLists.txt)
	add_library(CppUtils  STATIC IMPORTED)
else()
	add_library(mutua::CppUtils ALIAS CppUtils)
endif()

add_executable(CppUtilsSpikes CppUtilsSpikes.cpp)
target_link_libraries(CppUtilsSpikes PRIVATE mutua::CppUtils)

enable_testing()
add_test(NAME "${PROJECT_NAME}" COMMAND "./CppUtilsSpikes")


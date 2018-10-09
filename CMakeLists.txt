# Usage:
#	TYPE="Debug";   rm -fr $TYPE; mkdir $TYPE; cd $TYPE; cmake -G Ninja -DCMAKE_BUILD_TYPE=$TYPE ..; cmake --build .; ctest; cd ..; ls -l "$TYPE/tests/cpp/CppUtilsSpikes"
#	TYPE="Release"; rm -fr $TYPE; mkdir $TYPE; cd $TYPE; cmake -G Ninja -DCMAKE_BUILD_TYPE=$TYPE ..; cmake --build .; ctest; cd ..; ls -l "$TYPE/tests/cpp/CppUtilsSpikes"

cmake_minimum_required (VERSION 3.9)
project (CppUtils VERSION 2018.10.09)

set(CMAKE_CXX_STANDARD 17)


enable_testing()
add_executable(CppUtilsSpikes
               "tests/cpp/CppUtilsSpikes.cpp"
	       "cpp/TimeMeasurements.h" "cpp/TimeMeasurements.cpp"
)
add_test(NAME "${PROJECT_NAME}" COMMAND "./CppUtilsSpikes")


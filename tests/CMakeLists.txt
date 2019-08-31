# test this CMakeLists.txt in a standalone fashion as:
#	TYPE="Release"; rm -fr $TYPE; mkdir $TYPE; cd $TYPE; cmake -G Ninja -DCMAKE_BUILD_TYPE=$TYPE ..; cmake --build .; ctest; cd ..; ls -l "$TYPE/CppUtilsSpikes"
# The rules here should be used when importing this library into other projects

cmake_minimum_required (VERSION 3.9)
project                (MTLSpikes VERSION 2019.08.30)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS    "${CMAKE_CXX_FLAGS} -march=native -mcpu=native -mtune=native")	# binary is optimized, but machine dependent

# increase the limit for constexpr loops
#set(CMAKE_CXX_FLAGS    "${CMAKE_CXX_FLAGS} -fconstexpr-steps=208896000")		# for clang 12*17*1024000 for constexpr 1M, 16 char keys	-- requires 5,6G RAM
#set(CMAKE_CXX_FLAGS    "${CMAKE_CXX_FLAGS} -fconstexpr-loop-limit=17409000")	# for gcc, 1024000*17+1000 for constexpr 1M, 16 char keys	-- requires more than 7G of RAM


file(GLOB_RECURSE SOURCE_FILES  cpp/*.h cpp/*.cpp)
message("Building executable './${CMAKE_BUILD_TYPE}/${PROJECT_NAME}' with: ${SOURCE_FILES}")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# imported mutua libraries
##########################
# mutua libraries have the inclues inside the "cpp/" directory

foreach (_referencedLib MTL)
	set(_referencedLib_SOURCES "../../${_referencedLib}/cpp/")
	message("    --> searching library '${_referencedLib}' in '~/.cmake/packages/${_referencedLib}' with headers in '${_referencedLib_SOURCES}'")
	if (NOT TARGET ${_referencedLib})
		find_package(${_referencedLib} CONFIG REQUIRED)
		add_library (${_referencedLib} STATIC IMPORTED GLOBAL)
		message("        (built in '${${_referencedLib}_DIR}')")
	else()
		add_library(mutua::${_referencedLib} ALIAS ${_referencedLib})
		message("        (defined by the parent CMakeLists.txt)")
	endif()
	include_directories("${_referencedLib_SOURCES}")
	target_link_libraries(${PROJECT_NAME} PRIVATE mutua::${_referencedLib})
endforeach()

enable_testing()
add_test(NAME "${PROJECT_NAME}" COMMAND "./${PROJECT_NAME}")

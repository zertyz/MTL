# Usage:
#	TYPE="Debug";   rm -fr $TYPE; mkdir $TYPE; cd $TYPE; cmake -G Ninja -DCMAKE_BUILD_TYPE=$TYPE ..; cmake --build .; ctest; cd ..; ls -l "$TYPE/tests/CppUtilsSpikes"
#	TYPE="Release"; rm -fr $TYPE; mkdir $TYPE; cd $TYPE; cmake -G Ninja -DCMAKE_BUILD_TYPE=$TYPE ..; cmake --build .; ctest; cd ..; ls -l "$TYPE/tests/CppUtilsSpikes"

cmake_minimum_required (VERSION 3.9)
project                (CppUtils VERSION 2018.10.09)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS    "${CMAKE_CXX_FLAGS} -march=native -mcpu=native -mtune=native")	# binary is optimized, but machine dependent


# imported mutua libraries
##########################
# mutua libraries have the inclues inside the "cpp/" directory

foreach (_referencedLib ) # no referenced libs on this project
  set(_referencedLib_SOURCES "../${_referencedLib}/cpp/")
  message("    --> including library '${_referencedLib}' with headers in '${_referencedLib_SOURCES}'")
  if (NOT TARGET ${_referencedLib})
    find_package(${_referencedLib} CONFIG REQUIRED)
    add_library (${_referencedLib} STATIC IMPORTED GLOBAL)
    message("        (built in '${${_referencedLib}_DIR}')")
  else()
    add_library(mutua::${_referencedLib} ALIAS ${_referencedLib})
    message("        (defined by the parent CMakeLists.txt)")
  endif()
  include_directories("${_referencedLib_SOURCES}")
endforeach()


# this lib
##########

file(GLOB_RECURSE CPP_FILES cpp/*.cpp)
file(GLOB_RECURSE H_FILES   cpp/*.h)
message("Building '${PROJECT_NAME}' library with:")
message("      .h files: [${H_FILES}]")
message("    .cpp files: [${CPP_FILES}]")
add_library(${PROJECT_NAME} ${H_FILES} ${CPP_FILES})

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PUBLIC "${PROJECT_NAME}_DEBUG=$<CONFIG:Debug>")
#target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} INTERFACE
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cpp/>  
                           $<INSTALL_INTERFACE:cpp/>)                             # <prefix>/cpp/*.h

install(TARGETS ${PROJECT_NAME}  EXPORT      ${PROJECT_NAME}Targets
        LIBRARY                  DESTINATION lib
        ARCHIVE                  DESTINATION lib
        RUNTIME                  DESTINATION bin
        INCLUDES                 DESTINATION include)
install(FILES   ${H_FILES}       DESTINATION include COMPONENT Devel)

include(CMakePackageConfigHelpers)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
                                 VERSION ${Upstream_VERSION}
                                 COMPATIBILITY AnyNewerVersion
)

export(EXPORT ${PROJECT_NAME}Targets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}Targets.cmake"
       NAMESPACE mutua::)

configure_file(cmake/${PROJECT_NAME}Config.cmake
               "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake"
               COPYONLY)

set(ConfigPackageLocation lib/cmake/${PROJECT_NAME})
install(EXPORT      ${PROJECT_NAME}Targets
        FILE        ${PROJECT_NAME}Targets.cmake
        NAMESPACE   mutua::
        DESTINATION ${ConfigPackageLocation})
install(FILES       cmake/${PROJECT_NAME}Config.cmake "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${ConfigPackageLocation}
        COMPONENT   Devel)

# export to user registry at ~/.cmake, so all other projects can refer to it with "find_package"
export(PACKAGE ${PROJECT_NAME})

enable_testing()
add_subdirectory("tests/")

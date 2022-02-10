cmake_minimum_required(VERSION 3.16)

project(mishegos_ghidra)

# Pull in the CMake sleigh build support
add_subdirectory(sleigh-cmake EXCLUDE_FROM_ALL)

add_library(mishegos_ghidra SHARED ghidra.cc sleighMishegos.cc)
set_target_properties(mishegos_ghidra
  PROPERTIES
    OUTPUT_NAME ghidra
    PREFIX ""
)

target_link_libraries(mishegos_ghidra PRIVATE sleigh::sla)

# TODO: Not sure how to get this into the project for only linking against the
# mishegos shared library
get_filename_component(mishegos_include_dir "../../include"
  ABSOLUTE BASE_DIR ${PROJECT_SOURCE_DIRECTORY}
)
target_include_directories(mishegos_ghidra
  PRIVATE "$<BUILD_INTERFACE:${mishegos_include_dir}>"
)

add_dependencies(mishegos_ghidra
  sleigh_spec_x86-64
)

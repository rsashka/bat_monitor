# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bat_monitor)

idf_build_set_property(COMPILE_OPTIONS "-DLWIP_SNMP" APPEND)
# idf_build_set_property(COMPILE_OPTIONS "-fexceptions" APPEND)
# idf_build_set_property(COMPILE_OPTIONS "-fsanitize=undefined" APPEND)

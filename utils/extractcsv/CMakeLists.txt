cmake_minimum_required(VERSION 3.1.1)
project(extractcsv)

#
# The version number.
#

#execute_process(COMMAND ${CMAKE_CURRENT_LIST_DIR}/../etc/version.sh "--major" OUTPUT_VARIABLE hft_VERSION_MAJOR)
#execute_process(COMMAND ${CMAKE_CURRENT_LIST_DIR}/../etc/version.sh "--minor" OUTPUT_VARIABLE hft_VERSION_MINOR)

#
# Set ‘make install’ destination dir.
#

# XXX Ciekawe info na temat robienia instalacji
# https://cmake.org/cmake/help/latest/command/install.html
set(CMAKE_INSTALL_PREFIX /tmp)

#
# Header .h
#

list(APPEND HEADERS
     ${PROJECT_SOURCE_DIR}/export_csv.h
     ${PROJECT_SOURCE_DIR}/timestamp.h
     ${PROJECT_SOURCE_DIR}/zipped_csv.h
)

#
# Sources .c
#

list(APPEND SOURCES
     ${PROJECT_SOURCE_DIR}/export_csv.c
     ${PROJECT_SOURCE_DIR}/timestamp.c
     ${PROJECT_SOURCE_DIR}/zipped_csv.c
     ${PROJECT_SOURCE_DIR}/main.c
)

add_executable(extractcsv ${HEADERS} ${SOURCES})
install(TARGETS extractcsv DESTINATION ${CMAKE_INSTALL_PREFIX})

#
# Setup nonstandard packages cmake script directory.
#

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#
# Check for dependences: find ZIP library (Required).
#

find_package(LibZip REQUIRED)
include_directories(${LIBZIP_INCLUDE_DIR})
target_link_libraries(extractcsv ${LIBZIP_LIBRARY})

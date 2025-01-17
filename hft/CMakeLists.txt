#          _____                    _____                _____
#         /\    \                  /\    \              /\    \
#        /::\____\                /::\    \            /::\    \
#       /:::/    /               /::::\    \           \:::\    \
#      /:::/    /               /::::::\    \           \:::\    \
#     /:::/    /               /:::/\:::\    \           \:::\    \
#    /:::/____/               /:::/__\:::\    \           \:::\    \
#   /::::\    \              /::::\   \:::\    \          /::::\    \
#  /::::::\    \   _____    /::::::\   \:::\    \        /::::::\    \
# /:::/\:::\    \ /\    \  /:::/\:::\   \:::\    \      /:::/\:::\    \
#/:::/  \:::\    /::\____\/:::/  \:::\   \:::\____\    /:::/  \:::\____\
#\::/    \:::\  /:::/    /\::/    \:::\   \::/    /   /:::/    \::/    /
# \/____/ \:::\/:::/    /  \/____/ \:::\   \/____/   /:::/    / \/____/
#          \::::::/    /            \:::\    \      /:::/    /
#           \::::/    /              \:::\____\    /:::/    /
#           /:::/    /                \::/    /    \::/    /
#          /:::/    /                  \/____/      \/____/
#         /:::/    /
#        /:::/    /      -=≡≣ High Frequency Trading System ® ≣≡=-
#        \::/    /
#         \/____/    Copyright © 2017 - 2025 by LLG Ryszard Gradowski
#                                All Rights Reserved.
#
#
# CAUTION! This application is an intellectual property
#          of LLG Ryszard Gradowski. This application as
#          well as any part of source code cannot be used,
#          modified and distributed by third party person
#          without prior written permission issued by
#          intellectual property owner.
#
# La Card - ... Za Tebe

cmake_minimum_required(VERSION 3.1.1)
project(hft)

#
# The version number.
#

execute_process(COMMAND ${CMAKE_CURRENT_LIST_DIR}/../etc/version.sh "--major" OUTPUT_VARIABLE hft_VERSION_MAJOR)
execute_process(COMMAND ${CMAKE_CURRENT_LIST_DIR}/../etc/version.sh "--minor" OUTPUT_VARIABLE hft_VERSION_MINOR)

#
# Set ‘make install’ destination dir.
#

# XXX Ciekawe info na temat robienia instalacji
# https://cmake.org/cmake/help/latest/command/install.html
set(CMAKE_INSTALL_PREFIX /tmp)

#
# Configure a header file to pass some of
# the CMake settings to the source code.
#

configure_file(
  "${PROJECT_SOURCE_DIR}/hft-config.h.in"
  "${PROJECT_SOURCE_DIR}/hft-config.h"
  )

#
# Header .h and .hpp.
#

list(APPEND HEADERS
     ${PROJECT_SOURCE_DIR}/hft-config.h
     ${PROJECT_SOURCE_DIR}/server/include/custom_except.hpp
     ${PROJECT_SOURCE_DIR}/server/include/deallocator.hpp
     ${PROJECT_SOURCE_DIR}/server/include/synchronized_queue.hpp
     ${PROJECT_SOURCE_DIR}/server/include/svr.hpp
     ${PROJECT_SOURCE_DIR}/server/include/thread_worker.hpp
     ${PROJECT_SOURCE_DIR}/server/include/utilities.hpp
     ${PROJECT_SOURCE_DIR}/server/include/curlpp.hpp
     ${PROJECT_SOURCE_DIR}/server/include/sms_alert.hpp
     ${PROJECT_SOURCE_DIR}/server/include/sms_messenger.hpp
     ${PROJECT_SOURCE_DIR}/server/include/daemon_process.hpp
     ${PROJECT_SOURCE_DIR}/server/include/marketplace_gateway_process.hpp
     ${PROJECT_SOURCE_DIR}/server/include/hft_request.hpp
     ${PROJECT_SOURCE_DIR}/server/include/hft_response.hpp
     ${PROJECT_SOURCE_DIR}/server/include/session_transport.hpp
     ${PROJECT_SOURCE_DIR}/server/include/basic_tcp_server.hpp
     ${PROJECT_SOURCE_DIR}/server/include/hft_server_config.hpp
     ${PROJECT_SOURCE_DIR}/server/include/hft_session.hpp
     ${PROJECT_SOURCE_DIR}/server/include/hft_session_state.hpp
     ${PROJECT_SOURCE_DIR}/server/include/trade_time_frame.hpp
     ${PROJECT_SOURCE_DIR}/server/include/instrument_handler.hpp
     ${PROJECT_SOURCE_DIR}/server/include/hft_handler_resource.hpp
     ${PROJECT_SOURCE_DIR}/server/include/hft_ih_dummy.hpp
     ${PROJECT_SOURCE_DIR}/server/include/metrics.hpp
     ${PROJECT_SOURCE_DIR}/forex-emulator/include/hft_instrument_property.hpp
     ${PROJECT_SOURCE_DIR}/forex-emulator/include/csv_loader.hpp
     ${PROJECT_SOURCE_DIR}/forex-emulator/include/csv_data_supplier.hpp
     ${PROJECT_SOURCE_DIR}/forex-emulator/include/hft_server_connector.hpp
     ${PROJECT_SOURCE_DIR}/forex-emulator/include/hft_forex_emulator.hpp
     ${PROJECT_SOURCE_DIR}/forex-emulator/include/hft_display_filter.hpp
)

#
# Sources .c and .cpp.
#

list(APPEND SOURCES
     ${PROJECT_SOURCE_DIR}/main.cpp
     ${PROJECT_SOURCE_DIR}/draft_main.cpp
     ${PROJECT_SOURCE_DIR}/server/deallocator.cpp
     ${PROJECT_SOURCE_DIR}/server/svr.cpp
     ${PROJECT_SOURCE_DIR}/server/utilities.cpp
     ${PROJECT_SOURCE_DIR}/server/curlpp.cpp
     ${PROJECT_SOURCE_DIR}/server/sms_alert.cpp
     ${PROJECT_SOURCE_DIR}/server/sms_messenger.cpp
     ${PROJECT_SOURCE_DIR}/server/daemon_process.cpp
     ${PROJECT_SOURCE_DIR}/server/marketplace_gateway_process.cpp
     ${PROJECT_SOURCE_DIR}/server/hft_server_config.cpp
     ${PROJECT_SOURCE_DIR}/server/hft_server_main.cpp
     ${PROJECT_SOURCE_DIR}/server/hft_request.cpp
     ${PROJECT_SOURCE_DIR}/server/hft_response.cpp
     ${PROJECT_SOURCE_DIR}/server/session_transport.cpp
     ${PROJECT_SOURCE_DIR}/server/hft_session.cpp
     ${PROJECT_SOURCE_DIR}/server/hft_session_state.cpp
     ${PROJECT_SOURCE_DIR}/server/trade_time_frame.cpp
     ${PROJECT_SOURCE_DIR}/server/instrument_handler.cpp
     ${PROJECT_SOURCE_DIR}/server/hft_handler_resource.cpp
     ${PROJECT_SOURCE_DIR}/server/hft_ih_dummy.cpp
     ${PROJECT_SOURCE_DIR}/server/metrics.cpp
     ${PROJECT_SOURCE_DIR}/forex-emulator/hft_forexemu_main.cpp
     ${PROJECT_SOURCE_DIR}/forex-emulator/hft_instrument_property.cpp
     ${PROJECT_SOURCE_DIR}/forex-emulator/csv_loader.cpp
     ${PROJECT_SOURCE_DIR}/forex-emulator/csv_data_supplier.cpp
     ${PROJECT_SOURCE_DIR}/forex-emulator/hft_server_connector.cpp
     ${PROJECT_SOURCE_DIR}/forex-emulator/hft_forex_emulator.cpp
     ${PROJECT_SOURCE_DIR}/forex-emulator/hft_display_filter.cpp
     ${PROJECT_SOURCE_DIR}/instrument-stats/hft_instrument_stats.cpp
     ${PROJECT_SOURCE_DIR}/../3rd-party/easylogging++/easylogging++.cc
)

add_executable(hft ${HEADERS} ${SOURCES})
install(TARGETS hft DESTINATION ${CMAKE_INSTALL_PREFIX})
target_compile_features(hft PRIVATE cxx_range_for)

get_target_property(TEMP hft COMPILE_FLAGS)
if(TEMP STREQUAL "TEMP-NOTFOUND")
    SET(TEMP "") # set to empty string
else()
    SET(TEMP "${TEMP} ")
endif()

SET(TEMP "${TEMP} -DBOOST_SYSTEM_NO_DEPRECATED" )
SET(TEMP "${TEMP} -DBOOST_ERROR_CODE_HEADER_ONLY" )
SET(TEMP "${TEMP} -DBOOST_BIND_GLOBAL_PLACEHOLDERS" )

SET(TEMP "${TEMP} -DELPP_THREAD_SAFE" )
SET(TEMP "${TEMP} -DELPP_STL_LOGGING" )
SET(TEMP "${TEMP} -DELPP_BOOST_LOGGING" )
SET(TEMP "${TEMP} -DELPP_FEATURE_CRASH_LOG" )
SET(TEMP "${TEMP} -DELPP_FEATURE_ALL" )
SET(TEMP "${TEMP} -DELPP_NO_DEFAULT_LOG_FILE" )

set_target_properties(hft PROPERTIES COMPILE_FLAGS ${TEMP} )


#
# Setup nonstandard packages cmake script directory.
#

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#
# Check for dependences: find cURL library (Required).
#

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})
target_link_libraries(hft ${CURL_LIBRARIES})

#
# Check for dependences: pthread (Required).
#

find_package (Threads)
target_link_libraries (hft ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries (hft ${CMAKE_DL_LIBS})

#
# Static linking boost libraries.
# Linking order is important!
#

set(BOOST_LIBS_PATH "${PROJECT_SOURCE_DIR}/../3rd-party/boost_current_release/stage/lib/")
target_link_libraries(hft "${BOOST_LIBS_PATH}/libboost_atomic.a")
target_link_libraries(hft "${BOOST_LIBS_PATH}/libboost_chrono.a")
target_link_libraries(hft "${BOOST_LIBS_PATH}/libboost_date_time.a")
target_link_libraries(hft "${BOOST_LIBS_PATH}/libboost_system.a")
target_link_libraries(hft "${BOOST_LIBS_PATH}/libboost_thread.a")
target_link_libraries(hft "${BOOST_LIBS_PATH}/libboost_filesystem.a")
target_link_libraries(hft "${BOOST_LIBS_PATH}/libboost_program_options.a")

#
# Additional include directory.
#

include_directories("${PROJECT_SOURCE_DIR}/server/include")
include_directories("${PROJECT_SOURCE_DIR}/forex-emulator/include")
include_directories("${PROJECT_SOURCE_DIR}/../3rd-party")
include_directories("${PROJECT_SOURCE_DIR}/../3rd-party/boost")
include_directories("${PROJECT_SOURCE_DIR}/../3rd-party/easylogging++")

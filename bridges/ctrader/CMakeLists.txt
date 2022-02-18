#
#    █░░█ █▀▀ ▀▀█▀▀ █▀█ █▀▀ ▀▀█▀▀ █▀▀█ █▀▀█ █▀▀▄ █▀▀ █▀▀█ 
#    █▀▀█ █▀▀ ░░█░░ ░▄▀ █░░ ░░█░░ █▄▄▀ █▄▄█ █░░█ █▀▀ █▄▄▀ 
#    ▀░░▀ ▀░░ ░░▀░░ █▄▄ ▀▀▀ ░░▀░░ ▀░▀▀ ▀░░▀ ▀▀▀░ ▀▀▀ ▀░▀▀
#
#           HFT2CTRADER is the HFT component that
#           is the bridge between the HFT and the
#           cTrader proxy infrastructure.
#
# Copyright © 2017 - 2022 by LLG Ryszard Gradowski, All Rights Reserved.
#
#
# CAUTION! This application is an intellectual property
#          of LLG Ryszard Gradowski. This application as
#          well as any part of source code cannot be used,
#          modified and distributed by third party person
#          without prior written permission issued by
#          intellectual property owner.
#

cmake_minimum_required(VERSION 3.1.1)
project(hft2ctrader)

#
# The version number.
#

execute_process(COMMAND ${CMAKE_CURRENT_LIST_DIR}/../../etc/version.sh "--major" OUTPUT_VARIABLE hft2ctrader_VERSION_MAJOR)
execute_process(COMMAND ${CMAKE_CURRENT_LIST_DIR}/../../etc/version.sh "--minor" OUTPUT_VARIABLE hft2ctrader_VERSION_MINOR)

set(CMAKE_INSTALL_PREFIX /var/lib/hft/marketplace-gateways/ctrader)

configure_file(
    "${PROJECT_SOURCE_DIR}/hft2ctrader-config.h.in"
    "${PROJECT_SOURCE_DIR}/hft2ctrader-config.h"
)

#
# Header .h and .hpp.
#

list(APPEND HEADERS
     ${PROJECT_SOURCE_DIR}/hft2ctrader-config.h
     ${PROJECT_SOURCE_DIR}/bridge/include/aux_functions.hpp
     ${PROJECT_SOURCE_DIR}/bridge/include/market_types.hpp
     ${PROJECT_SOURCE_DIR}/bridge/include/hft2ctrader_config.hpp
     ${PROJECT_SOURCE_DIR}/bridge/include/ctrader_ssl_connection.hpp
     ${PROJECT_SOURCE_DIR}/bridge/include/ctrader_api.hpp
     ${PROJECT_SOURCE_DIR}/bridge/include/proxy_core.hpp
     ${PROJECT_SOURCE_DIR}/bridge/include/hft_api.hpp
     ${PROJECT_SOURCE_DIR}/bridge/include/hft_connection.hpp
     ${PROJECT_SOURCE_DIR}/bridge/include/proxy_session.hpp
     ${PROJECT_SOURCE_DIR}/bridge/include/bridge.hpp
)

#
# Sources .c and .cpp.
#

list(APPEND SOURCES
     ${PROJECT_SOURCE_DIR}/main.cpp
     ${PROJECT_SOURCE_DIR}/draft_main.cpp
     ${PROJECT_SOURCE_DIR}/bridge/aux_functions.cpp
     ${PROJECT_SOURCE_DIR}/bridge/hft2ctrade_bridge_main.cpp
     ${PROJECT_SOURCE_DIR}/bridge/hft2ctrader_config.cpp
     ${PROJECT_SOURCE_DIR}/bridge/ctrader_ssl_connection.cpp
     ${PROJECT_SOURCE_DIR}/bridge/ctrader_api.cpp
     ${PROJECT_SOURCE_DIR}/bridge/hft_api.cpp
     ${PROJECT_SOURCE_DIR}/bridge/proxy_core.cpp
     ${PROJECT_SOURCE_DIR}/bridge/hft_connection.cpp
     ${PROJECT_SOURCE_DIR}/bridge/proxy_session.cpp
     ${PROJECT_SOURCE_DIR}/history-ticks/history_ticks_main.cpp
     ${PROJECT_SOURCE_DIR}/../../3rd-party/easylogging++/easylogging++.cc
)

#
# Check for additional dependences.
#

find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
    openapi-proto/OpenApiModelMessages.proto
    openapi-proto/OpenApiMessages.proto
    openapi-proto/OpenApiCommonModelMessages.proto
    openapi-proto/OpenApiCommonMessages.proto
)
add_executable(hft2ctrader ${SOURCES} ${PROTO_SRCS} ${HEADERS} ${PROTO_HDRS})
target_link_libraries(hft2ctrader ${Protobuf_LIBRARIES})

find_package(Threads REQUIRED)
target_link_libraries (hft2ctrader ${CMAKE_THREAD_LIBS_INIT})

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(hft2ctrader ${OPENSSL_LIBRARIES})

install(TARGETS hft2ctrader DESTINATION ${CMAKE_INSTALL_PREFIX})
target_compile_features(hft2ctrader PRIVATE cxx_range_for)

get_target_property(TEMP hft2ctrader COMPILE_FLAGS)

if(TEMP STREQUAL "TEMP-NOTFOUND")
    SET(TEMP "") # set to empty string
else()
    SET(TEMP "${TEMP} ")
endif()

SET(TEMP "${TEMP} -DBOOST_SYSTEM_NO_DEPRECATED" )
SET(TEMP "${TEMP} -DBOOST_ERROR_CODE_HEADER_ONLY" )
SET(TEMP "${TEMP} -DBOOST_BIND_GLOBAL_PLACEHOLDERS" )

SET(TEMP "${TEMP} -DBOOST_MPL_CFG_NO_PREPROCESSED_HEADERS" )
SET(TEMP "${TEMP} -DBOOST_MPL_LIMIT_VECTOR_SIZE=30" )
SET(TEMP "${TEMP} -DBOOST_MPL_LIMIT_MAP_SIZE=30" )

SET(TEMP "${TEMP} -DELPP_THREAD_SAFE" )
SET(TEMP "${TEMP} -DELPP_STL_LOGGING" )
SET(TEMP "${TEMP} -DELPP_BOOST_LOGGING" )
SET(TEMP "${TEMP} -DELPP_FEATURE_CRASH_LOG" )
SET(TEMP "${TEMP} -DELPP_FEATURE_ALL" )
SET(TEMP "${TEMP} -DELPP_NO_DEFAULT_LOG_FILE" )

set_target_properties(hft2ctrader PROPERTIES COMPILE_FLAGS ${TEMP} )

#
# Static linking boost libraries.
# Linking order is important!
#

set(BOOST_LIBS_PATH "${PROJECT_SOURCE_DIR}/../../3rd-party/boost_current_release/stage/lib/")
target_link_libraries(hft2ctrader "${BOOST_LIBS_PATH}/libboost_atomic.a")
target_link_libraries(hft2ctrader "${BOOST_LIBS_PATH}/libboost_chrono.a")
target_link_libraries(hft2ctrader "${BOOST_LIBS_PATH}/libboost_date_time.a")
target_link_libraries(hft2ctrader "${BOOST_LIBS_PATH}/libboost_system.a")
target_link_libraries(hft2ctrader "${BOOST_LIBS_PATH}/libboost_thread.a")
target_link_libraries(hft2ctrader "${BOOST_LIBS_PATH}/libboost_filesystem.a")
target_link_libraries(hft2ctrader "${BOOST_LIBS_PATH}/libboost_program_options.a")

#
# Additional include directory.
#

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/bridge/include")
include_directories("${PROJECT_SOURCE_DIR}/../../3rd-party")
include_directories("${PROJECT_SOURCE_DIR}/../../3rd-party/boost")
include_directories("${PROJECT_SOURCE_DIR}/../../3rd-party/easylogging++")

cmake_minimum_required(VERSION 3.1.1)
project(simple_tracker VERSION 1.0.0 DESCRIPTION "HFT instrument handler plugin")

set(CMAKE_INSTALL_LIBDIR /var/lib/hft/instrument-handlers)

list(APPEND HEADERS
     ${PROJECT_SOURCE_DIR}/include/simple_tracker.hpp
)

list(APPEND SOURCES
     ${PROJECT_SOURCE_DIR}/simple_tracker.cpp
)

add_library(simple_tracker SHARED ${SOURCES} ${HEADERS})
install(TARGETS simple_tracker DESTINATION ${CMAKE_INSTALL_LIBDIR})

target_compile_features(simple_tracker PRIVATE cxx_range_for)

get_target_property(TEMP simple_tracker COMPILE_FLAGS)
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

set_target_properties(simple_tracker PROPERTIES COMPILE_FLAGS ${TEMP} )
set_target_properties(simple_tracker PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1)

#
# Check for dependences: pthread (Required).
#

find_package (Threads)
target_link_libraries (simple_tracker ${CMAKE_THREAD_LIBS_INIT})

#
# Additional include directory.
#

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/../../hft/server/include")
include_directories("${PROJECT_SOURCE_DIR}/../../3rd-party")
include_directories("${PROJECT_SOURCE_DIR}/../../3rd-party/boost")
include_directories("${PROJECT_SOURCE_DIR}/../../3rd-party/easylogging++")

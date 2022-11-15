cmake_minimum_required(VERSION 3.1.1)
project(trend_tracker VERSION 1.0.0 DESCRIPTION "HFT instrument handler plugin")

set(CMAKE_INSTALL_LIBDIR /var/lib/hft/instrument-handlers)

list(APPEND HEADERS
     ${PROJECT_SOURCE_DIR}/include/trend_tracker.hpp
     ${PROJECT_SOURCE_DIR}/include/advice_types.hpp
     ${PROJECT_SOURCE_DIR}/include/interval_type.hpp
     ${PROJECT_SOURCE_DIR}/include/invalidable.hpp
     ${PROJECT_SOURCE_DIR}/include/game.hpp
     ${PROJECT_SOURCE_DIR}/include/exchange_rates_collector.hpp
     ${PROJECT_SOURCE_DIR}/include/interval_processor.hpp
     ${PROJECT_SOURCE_DIR}/include/strategic_engine.hpp
)

list(APPEND SOURCES
     ${PROJECT_SOURCE_DIR}/interval_type.cpp
     ${PROJECT_SOURCE_DIR}/trend_tracker.cpp
     ${PROJECT_SOURCE_DIR}/exchange_rates_collector.cpp
     ${PROJECT_SOURCE_DIR}/game.cpp
     ${PROJECT_SOURCE_DIR}/interval_processor.cpp
     ${PROJECT_SOURCE_DIR}/strategic_engine.cpp
)

add_library(trend_tracker SHARED ${SOURCES} ${HEADERS})
install(TARGETS trend_tracker DESTINATION ${CMAKE_INSTALL_LIBDIR})

target_compile_features(trend_tracker PRIVATE cxx_range_for)

get_target_property(TEMP trend_tracker COMPILE_FLAGS)
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

set_target_properties(trend_tracker PROPERTIES COMPILE_FLAGS ${TEMP} )
set_target_properties(trend_tracker PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1)

#
# Check for dependences: pthread (Required).
#

find_package (Threads)
target_link_libraries (trend_tracker ${CMAKE_THREAD_LIBS_INIT})

#
# Additional include directory.
#

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/../../hft/server/include")
include_directories("${PROJECT_SOURCE_DIR}/../../3rd-party")
include_directories("${PROJECT_SOURCE_DIR}/../../3rd-party/boost")
include_directories("${PROJECT_SOURCE_DIR}/../../3rd-party/easylogging++")

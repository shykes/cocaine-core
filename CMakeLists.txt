CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(COCAINE-CORE)

SET(COCAINE_VERSION_MAJOR   0)
SET(COCAINE_VERSION_MINOR   10)
SET(COCAINE_VERSION_RELEASE 6)

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

CONFIGURE_FILE(
    "${PROJECT_SOURCE_DIR}/config.hpp.in"
    "${PROJECT_SOURCE_DIR}/include/cocaine/config.hpp")

FIND_PACKAGE(Boost 1.40.0
    REQUIRED COMPONENTS
        system
        filesystem
        program_options)

IF(NOT APPLE)
    FIND_PACKAGE(OpenSSL REQUIRED)
    SET(CRYPTO_SOURCES "src/auth")
    SET(CRYPTO_LIBRARY "crypto")
ENDIF()

INCLUDE(cmake/locate_library.cmake)

LOCATE_LIBRARY(LIBEV "ev++.h" "ev" "libev")
LOCATE_LIBRARY(LIBMSGPACK "msgpack.hpp" "msgpack")
LOCATE_LIBRARY(LIBARCHIVE "archive.h" "archive")
LOCATE_LIBRARY(LIBLTDL "ltdl.h" "ltdl")

IF(NOT APPLE)
    LOCATE_LIBRARY(LIBUUID "uuid/uuid.h" "uuid")
    SET(LIBUUID_LIBRARY "uuid")
ENDIF()

IF(NOT APPLE)
    LOCATE_LIBRARY(LIBBFD "bfd.h" "bfd")
    SET(LIBBFD_LIBRARY "bfd" "dl")
ENDIF()

INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${LIBEV_INCLUDE_DIRS}
    ${LIBMSGPACK_INCLUDE_DIRS}
    ${LIBARCHIVE_INCLUDE_DIRS}
    ${LIBLTDL_INCLUDE_DIRS})

INCLUDE_DIRECTORIES(BEFORE
    ${PROJECT_SOURCE_DIR}/foreign/jsoncpp-0.6.0-rc2/include
    ${PROJECT_SOURCE_DIR}/foreign/backward-cpp
    ${PROJECT_SOURCE_DIR}/include)

LINK_DIRECTORIES(
    ${Boost_LIBRARY_DIRS}
    ${LIBEV_LIBRARY_DIRS}
    ${LIBMSGPACK_LIBRARY_DIRS}
    ${LIBARCHIVE_LIBRARY_DIRS}
    ${LIBLTDL_LIBRARY_DIRS})

ADD_LIBRARY(json
    foreign/jsoncpp-0.6.0-rc2/src/lib_json/json_value
    foreign/jsoncpp-0.6.0-rc2/src/lib_json/json_reader
    foreign/jsoncpp-0.6.0-rc2/src/lib_json/json_writer)

SET_TARGET_PROPERTIES(json PROPERTIES
    COMPILE_FLAGS "-fPIC")

ADD_LIBRARY(cocaine-core SHARED
    src/actor
    src/api
    src/app
    src/archive
    src/context
    ${CRYPTO_SOURCES}
    src/dispatch
    src/drivers/fs
    src/drivers/time
    src/engine
    src/essentials
    src/gateways/adhoc
    src/isolates/process
    src/locator
    src/loggers/files
    src/loggers/syslog
    src/logging
    src/manifest
    src/profile
    src/queue
    src/repository
    src/services/logging
    src/services/node
    src/services/storage
    src/session
    src/slave
    src/slot
    src/storages/files
    src/unique_id)

TARGET_LINK_LIBRARIES(cocaine-core
    archive
    boost_system-mt
    boost_filesystem-mt
    ${CRYPTO_LIBRARY}
    ev
    json
    ltdl
    msgpack
    ${LIBUUID_LIBRARY})

SET_TARGET_PROPERTIES(cocaine-core PROPERTIES
    VERSION 2)

ADD_EXECUTABLE(cocaine-runtime
    src/runtime/pid_file
    src/runtime/runtime)

TARGET_LINK_LIBRARIES(cocaine-runtime
    ${LIBBFD_LIBRARY}
    boost_program_options-mt
    cocaine-core)

SET_TARGET_PROPERTIES(cocaine-core cocaine-runtime PROPERTIES
    COMPILE_FLAGS "-std=c++0x -W -Wall -Werror -pedantic")

IF(NOT COCAINE_LIBDIR)
    SET(COCAINE_LIBDIR lib)
ENDIF()

INSTALL(
    TARGETS
        cocaine-core
        cocaine-runtime
        json
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION ${COCAINE_LIBDIR} COMPONENT runtime
    ARCHIVE DESTINATION ${COCAINE_LIBDIR} COMPONENT developement)

INSTALL(
    DIRECTORY
        include/
        foreign/jsoncpp-0.6.0-rc2/include/
    DESTINATION include
    COMPONENT development)

cmake_minimum_required (VERSION 2.6)
project (azouk-libraries)

# The version number.
set (PROGRAM_VERSION 1.0.0)

#set (CMAKE_CXX_FLAGS "-Wall -Wextra")
set (CMAKE_CXX_FLAGS
"-Wall -Wnon-virtual-dtor -Winit-self -Wswitch-enum -Wunused -Wunused-parameter")

# add the binary tree to the search path for include files
# so that we will find config.h
include_directories ("${PROJECT_BINARY_DIR}"
                     "${PROJECT_SOURCE_DIR}/src")

# look for protoc, the ProtoBuf compiler
find_program (PROTOC protoc)

# used for the version file
find_program (SHA1SUM sha1sum)
find_program (AWK awk)
find_program (CUT cut)



include (CheckIncludeFiles)
# usage: CHECK_INCLUDE_FILES (<header> <RESULT_VARIABLE> )

check_include_files (fcntl.h FCNTL_H)
check_include_files (stdlib.h HAVE_STLIB_H)
check_include_files (unistd.h HAVE_UNISTD_H)



find_library (PTHREAD_LIB pthread DOC "thread library" REQUIRED)
find_library (PROTOC_LIB protoc REQUIRED)
find_library (PROTOBUF_LIB protobuf REQUIRED)

set (LIBS ${PTHREAD_LIB} ${PROTOC_LIB} ${PROTOBUF_LIB})

# already released, but not known by my cmake distribution
set (Boost_ADDITIONAL_VERSIONS "1.42" "1.42.0")

find_package (Boost 1.35.0 REQUIRED)
include_directories (${Boost_INCLUDE_DIRS})

# set (Boost_USE_STATIC_LIBS   OFF)
set (Boost_USE_MULTITHREADED ON)
find_package (Boost 1.35.0 COMPONENTS program_options signals system REQUIRED)

list(APPEND LIBS ${Boost_LIBRARIES})



set(common_sources
  ${CMAKE_BINARY_DIR}/azlib/logging/Logging.pb.cc
  ${CMAKE_BINARY_DIR}/azlib/logging/Logging.pb.h
  src/azlib/signals.cc src/azlib/signals.h
  src/azlib/random.cc src/azlib/random.h
  src/azlib/logging.cc src/azlib/logging.h
  src/azlib/util/Assert.cc src/azlib/util/Assert.h
  src/azlib/util/Exception.cc src/azlib/util/Exception.h
  src/azouk/util/kwargs.cc src/azouk/util/kwargs.h
  src/azouk/xrelease.cc src/azouk/xrelease.h
  )

set(multiplexer_sources
  ${CMAKE_BINARY_DIR}/multiplexer/Multiplexer.pb.cc
  ${CMAKE_BINARY_DIR}/multiplexer/Multiplexer.pb.h
  )

set(mxcontrol_sources
  src/mxcontrol.cc
  src/mxcontrol-commands/TasksHolder.cc
  src/mxcontrol-commands/TasksHolder.h
  src/mxcontrol-commands/Task.cc
  src/mxcontrol-commands/Task.h
  src/mxcontrol-commands/StreamLogs.cc
  # src/mxcontrol-commands/StreamLogs.h
  src/mxcontrol-commands/StartMultiplexerServer.cc
  src/mxcontrol-commands/StartMultiplexerServer.h
  src/mxcontrol-commands/ReceiveLogs.cc
  # src/mxcontrol-commands/ReceiveLogs.h
  src/mxcontrol-commands/Help.cc
  src/mxcontrol-commands/Help.h
  src/multiplexer/BasicClient.cc
  src/multiplexer/BasicClient.h
  src/multiplexer/Client.cc
  src/multiplexer/Client.h
  src/multiplexer/Server.cc
  src/multiplexer/Server.h
  src/multiplexer/ConnectionsManager.cc
  src/multiplexer/ConnectionsManager.h
  src/multiplexer/io/RawMessage.cc
  src/multiplexer/io/RawMessage.h
  src/multiplexer/backend/BaseMultiplexerServer.cc
  src/multiplexer/backend/BaseMultiplexerServer.h
  ${CMAKE_CURRENT_BINARY_DIR}/multiplexer/multiplexer.constants.h
  )


include (CheckFunctionExists)
check_function_exists (gethostname HAVE_GETHOSTNAME)

if (NOT HAVE_GETHOSTNAME)
  list(APPEND mxcontrol_sources lib/gethostname.c)
endif ()

set (PROTOC_FLAGS "")

add_custom_command (
  OUTPUT ${CMAKE_BINARY_DIR}/azlib/logging/Logging.pb.cc
         ${CMAKE_BINARY_DIR}/azlib/logging/Logging.pb.h
  DEPENDS src/azlib/logging/Logging.proto
  COMMAND ${PROTOC} ${PROTOC_FLAGS}
          ${CMAKE_SOURCE_DIR}/src/azlib/logging/Logging.proto
          -I${CMAKE_CURRENT_SOURCE_DIR}/src
          --cpp_out=${CMAKE_BINARY_DIR}
)

add_custom_command (
  OUTPUT ${CMAKE_BINARY_DIR}/multiplexer/Multiplexer.pb.cc
         ${CMAKE_BINARY_DIR}/multiplexer/Multiplexer.pb.h
  DEPENDS src/multiplexer/Multiplexer.proto
  COMMAND ${PROTOC} ${PROTOC_FLAGS}
          ${CMAKE_SOURCE_DIR}/src/multiplexer/Multiplexer.proto
          -I${CMAKE_CURRENT_SOURCE_DIR}/src
          --cpp_out=${CMAKE_BINARY_DIR}
)

add_executable(mxcontrol
  ${common_sources} ${mxcontrol_sources} ${multiplexer_sources})
target_link_libraries(mxcontrol ${LIBS})

add_executable(generate_constants
  src/generate_constants.cc
  ${common_sources}
  ${multiplexer_sources}
  )
target_link_libraries(generate_constants ${LIBS})

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/multiplexer/multiplexer.constants.h
  DEPENDS generate_constants src/multiplexer.rules
  COMMAND generate_constants ${CMAKE_SOURCE_DIR}/src/multiplexer.rules
         ${CMAKE_CURRENT_BINARY_DIR}/multiplexer/multiplexer.constants.h
  COMMAND generate_constants ${CMAKE_SOURCE_DIR}/src/multiplexer.rules
         ${CMAKE_CURRENT_BINARY_DIR}/multiplexer/multiplexer_constants.py
  )

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/multiplexer/multiplexer_constants.py
  DEPENDS generate_constants src/multiplexer.rules
  COMMAND generate_constants ${CMAKE_SOURCE_DIR}/src/multiplexer.rules
         ${CMAKE_CURRENT_BINARY_DIR}/multiplexer/multiplexer_constants.py
  )



execute_process (
  COMMAND ${SHA1SUM}
  COMMAND ${AWK} "{print $1}"
  OUTPUT_VARIABLE logging_type_id_hash
  INPUT_FILE ${CMAKE_SOURCE_DIR}/src/logging.type_id.constants.txt
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )
message(STATUS "logging_type_id hash is |${logging_type_id_hash}|")
string(COMPARE EQUAL "${logging_type_id_hash}" "" logging_type_id_empty)
if (logging_type_id_empty)
  message(FATAL_ERROR "sha1sum failed")
endif ()

execute_process (
  COMMAND ${AWK} "/^[^#]/ {print \"\tstatic const unsigned int\", $0 \";\" }"
  OUTPUT_VARIABLE logging_type_id_body
  INPUT_FILE ${CMAKE_SOURCE_DIR}/src/logging.type_id.constants.txt
  )

configure_file (
  ${PROJECT_SOURCE_DIR}/src/azouk/logging/type_id_constants.h.in
  ${PROJECT_BINARY_DIR}/azouk/logging/type_id_constants.h
)

cmake_minimum_required(VERSION 3.14)
project(frictioncopilot)

include(FetchContent)

# Fetch gRPC
FetchContent_Declare(
  grpc
  GIT_REPOSITORY https://github.com/grpc/grpc
  GIT_TAG        v1.62.1
)

set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
set(gRPC_INSTALL OFF CACHE BOOL "Disable installation of gRPC" FORCE)
set(protobuf_INSTALL OFF CACHE BOOL "Disable installation of protobuf" FORCE)
set(ABSL_ENABLE_INSTALL OFF CACHE BOOL "Disable installation of absl" FORCE)
set(utf8_range_ENABLE_INSTALL OFF CACHE BOOL "Disable installation of utf8_range" FORCE)
set(gRPC_BUILD_TESTS OFF CACHE BOOL "Disable building tests" FORCE)
set(gRPC_ZLIB_PROVIDER "module" CACHE STRING "Use submodule zlib" FORCE)
set(gRPC_CARES_PROVIDER "module" CACHE STRING "Use submodule cares" FORCE)
set(gRPC_RE2_PROVIDER "module" CACHE STRING "Use submodule re2" FORCE)
set(gRPC_SSL_PROVIDER "module" CACHE STRING "Use submodule ssl" FORCE)
set(gRPC_PROTOBUF_PROVIDER "module" CACHE STRING "Use submodule protobuf" FORCE)
set(gRPC_ABSL_PROVIDER "module" CACHE STRING "Use submodule absl" FORCE)

FetchContent_MakeAvailable(grpc)

# Protocol file target
set(PROTO_SRC_DIR "${CMAKE_SOURCE_DIR}/../protos")
set(PROTOS "${PROTO_SRC_DIR}/copilot.proto")

# Output path
set(PROTO_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}")

set(PROTO_SRCS "${PROTO_BIN_DIR}/copilot.pb.cc")
set(PROTO_HDRS "${PROTO_BIN_DIR}/copilot.pb.h")
set(GRPC_SRCS "${PROTO_BIN_DIR}/copilot.grpc.pb.cc")
set(GRPC_HDRS "${PROTO_BIN_DIR}/copilot.grpc.pb.h")

# The protoc compiler and grpc_cpp_plugin will be built by gRPC's FetchContent
add_custom_command(
      OUTPUT "${PROTO_SRCS}" "${PROTO_HDRS}" "${GRPC_SRCS}" "${GRPC_HDRS}"
      COMMAND protobuf::protoc
      ARGS --grpc_out "${PROTO_BIN_DIR}"
           --cpp_out "${PROTO_BIN_DIR}"
           -I "${PROTO_SRC_DIR}"
           --plugin=protoc-gen-grpc=$<TARGET_FILE:grpc_cpp_plugin>
           "${PROTOS}"
      DEPENDS "${PROTOS}" protobuf::protoc grpc_cpp_plugin
)

add_library(copilot_client STATIC
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    ${GRPC_SRCS}
    ${GRPC_HDRS}
    CopilotClient.cpp
    CopilotClient.h
)

target_include_directories(copilot_client PUBLIC "${PROTO_BIN_DIR}")
target_link_libraries(copilot_client PUBLIC grpc++ protobuf::libprotobuf)

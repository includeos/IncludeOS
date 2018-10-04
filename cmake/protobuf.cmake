# Download and install Google Protobuf

cmake_minimum_required(VERSION 3.1.0)

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")

set(LIB_PROTOBUF ${INCLUDEOS_ROOT}/lib/protobuf/src)
set(PROTOBUF_SRC ${LIB_PROTOBUF}/google/protobuf)

include_directories(${LIB_PROTOBUF})
include_directories(${INCLUDEOS_ROOT}/api/posix)
include_directories(${LIBCXX_INCLUDE_DIR})
include_directories(${MUSL_INCLUDE_DIR})

# Maybe possible to use wildcard with files(...) to gather all cc objects.
set(PROTOBUF_SOURCES
  ${PROTOBUF_SRC}/any.cc
  ${PROTOBUF_SRC}/any.pb.cc
  ${PROTOBUF_SRC}/api.pb.cc
  ${PROTOBUF_SRC}/arena.cc
  ${PROTOBUF_SRC}/arenastring.cc
  ${PROTOBUF_SRC}/compiler/importer.cc
  ${PROTOBUF_SRC}/compiler/parser.cc
  ${PROTOBUF_SRC}/descriptor.cc
  ${PROTOBUF_SRC}/descriptor.pb.cc
  ${PROTOBUF_SRC}/descriptor_database.cc
  ${PROTOBUF_SRC}/duration.pb.cc
  ${PROTOBUF_SRC}/dynamic_message.cc
  ${PROTOBUF_SRC}/empty.pb.cc
  ${PROTOBUF_SRC}/extension_set.cc
  ${PROTOBUF_SRC}/extension_set_heavy.cc
  ${PROTOBUF_SRC}/field_mask.pb.cc
  ${PROTOBUF_SRC}/generated_message_reflection.cc
  ${PROTOBUF_SRC}/generated_message_table_driven_lite.cc
  ${PROTOBUF_SRC}/generated_message_table_driven.cc
  ${PROTOBUF_SRC}/generated_message_util.cc
  ${PROTOBUF_SRC}/io/coded_stream.cc
  ${PROTOBUF_SRC}/io/gzip_stream.cc
  ${PROTOBUF_SRC}/io/printer.cc
  ${PROTOBUF_SRC}/io/strtod.cc
  ${PROTOBUF_SRC}/io/tokenizer.cc
  ${PROTOBUF_SRC}/io/zero_copy_stream.cc
  ${PROTOBUF_SRC}/io/zero_copy_stream_impl_lite.cc
  ${PROTOBUF_SRC}/io/zero_copy_stream_impl.cc
  ${PROTOBUF_SRC}/map_field.cc
  ${PROTOBUF_SRC}/message_lite.cc
  ${PROTOBUF_SRC}/message.cc
  ${PROTOBUF_SRC}/reflection_ops.cc
  ${PROTOBUF_SRC}/repeated_field.cc
  ${PROTOBUF_SRC}/service.cc
  ${PROTOBUF_SRC}/source_context.pb.cc
  ${PROTOBUF_SRC}/struct.pb.cc
  ${PROTOBUF_SRC}/stubs/atomicops_internals_x86_gcc.cc
  ${PROTOBUF_SRC}/stubs/atomicops_internals_x86_msvc.cc
  ${PROTOBUF_SRC}/stubs/bytestream.cc
  ${PROTOBUF_SRC}/stubs/common.cc
  ${PROTOBUF_SRC}/stubs/int128.cc
  ${PROTOBUF_SRC}/stubs/io_win32.cc
  ${PROTOBUF_SRC}/stubs/mathlimits.cc
  ${PROTOBUF_SRC}/stubs/once.cc
  ${PROTOBUF_SRC}/stubs/status.cc
  ${PROTOBUF_SRC}/stubs/statusor.cc
  ${PROTOBUF_SRC}/stubs/stringpiece.cc
  ${PROTOBUF_SRC}/stubs/stringprintf.cc
  ${PROTOBUF_SRC}/stubs/structurally_valid.cc
  ${PROTOBUF_SRC}/stubs/strutil.cc
  ${PROTOBUF_SRC}/stubs/time.cc
  ${PROTOBUF_SRC}/stubs/substitute.cc
  ${PROTOBUF_SRC}/text_format.cc
  ${PROTOBUF_SRC}/timestamp.pb.cc
  ${PROTOBUF_SRC}/type.pb.cc
  ${PROTOBUF_SRC}/unknown_field_set.cc
  ${PROTOBUF_SRC}/util/delimited_message_util.cc
  ${PROTOBUF_SRC}/util/field_comparator.cc
  ${PROTOBUF_SRC}/util/field_mask_util.cc
  ${PROTOBUF_SRC}/util/internal/datapiece.cc
  ${PROTOBUF_SRC}/util/internal/default_value_objectwriter.cc
  ${PROTOBUF_SRC}/util/internal/error_listener.cc
  ${PROTOBUF_SRC}/util/internal/field_mask_utility.cc
  ${PROTOBUF_SRC}/util/internal/json_escaping.cc
  ${PROTOBUF_SRC}/util/internal/json_objectwriter.cc
  ${PROTOBUF_SRC}/util/internal/json_stream_parser.cc
  ${PROTOBUF_SRC}/util/internal/object_writer.cc
  ${PROTOBUF_SRC}/util/internal/proto_writer.cc
  ${PROTOBUF_SRC}/util/internal/protostream_objectsource.cc
  ${PROTOBUF_SRC}/util/internal/protostream_objectwriter.cc
  ${PROTOBUF_SRC}/util/internal/type_info.cc
  ${PROTOBUF_SRC}/util/internal/type_info_test_helper.cc
  ${PROTOBUF_SRC}/util/internal/utility.cc
  ${PROTOBUF_SRC}/util/json_util.cc
  ${PROTOBUF_SRC}/util/message_differencer.cc
  ${PROTOBUF_SRC}/util/time_util.cc
  ${PROTOBUF_SRC}/util/type_resolver_util.cc
  ${PROTOBUF_SRC}/wire_format_lite.cc
  ${PROTOBUF_SRC}/wire_format.cc
  ${PROTOBUF_SRC}/wrappers.pb.cc
)

add_library(protobuf STATIC ${PROTOBUF_SOURCES})
target_compile_definitions(protobuf PRIVATE HAVE_PTHREAD=0)
target_compile_options(protobuf PRIVATE -Wno-sign-compare -Wno-unused-parameter)

# Make sure precompiled libraries exists
add_dependencies(protobuf PrecompiledLibraries)

# Install library
install(TARGETS protobuf DESTINATION includeos/${ARCH}/lib)

# Install headers
install(DIRECTORY ${LIB_PROTOBUF}/google
  DESTINATION includeos/include
  FILES_MATCHING PATTERN "*.h"
  PATTERN "protobuf/testdata" EXCLUDE
  PATTERN "protobuf/testing" EXCLUDE)

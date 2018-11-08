cmake_minimum_required(VERSION 2.8.9)

if (${ARCH} STREQUAL "x86_64")
  add_definitions(-DARCH_${ARCH})
  add_definitions(-DARCH="${ARCH}")

  # microLB static library
  add_library(microlb STATIC
      micro_lb/autoconf.cpp
      micro_lb/balancer.cpp
      micro_lb/openssl.cpp
      micro_lb/serialize.cpp
    )

  add_dependencies(microlb PrecompiledLibraries s2n_bundle)

  target_include_directories(microlb PUBLIC ${INCLUDEOS_ROOT}/api/posix)
  target_include_directories(microlb PUBLIC ${LIBCXX_INCLUDE_DIR})
  target_include_directories(microlb PUBLIC ${MUSL_INCLUDE_DIR})
  target_include_directories(microlb PUBLIC ${INCLUDEOS_ROOT}/src/include)
  target_include_directories(microlb PUBLIC ${INCLUDEOS_ROOT}/api)
  target_include_directories(microlb PUBLIC ${INCLUDEOS_ROOT}/lib/LiveUpdate)
  target_include_directories(microlb PUBLIC ${INCLUDEOS_ROOT}/mod/rapidjson/include)
  target_include_directories(microlb PUBLIC ${INCLUDEOS_ROOT}/mod/GSL/)
  target_include_directories(microlb PUBLIC ${S2N_INCLUDE})

  install(TARGETS microlb DESTINATION includeos/${ARCH}/lib)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/micro_lb/balancer.hpp DESTINATION includeos/include/micro_lb)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/microLB DESTINATION includeos/include)
endif()

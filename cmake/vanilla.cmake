IF(${ARCH} STREQUAL "x86_64" OR ${ARCH} STREQUAL "i686")
  set(CAPABS "-msse3 -mfpmath=sse")
  message(STATUS "Using vanilla CPU features: SSE3. CAPABS = ${CAPABS}")
ENDIF()

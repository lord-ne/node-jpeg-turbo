# This file is added as CMAKE_PROJECT_INCLUDE for the libjpeg-turbo ExternalProject.
# We also include it in the main project to ensure consistency

if(MSVC)
  set(EXCEPTIONS_FLAG "/EHs")
else()
  # We use the GNU -fexceptions flag on anything other than MSVC. This is supported
  # by GCC and Clang, which covers anything we're likely to be building with.
  set(EXCEPTIONS_FLAG "-fexceptions")
endif()

message(STATUS "include.cmake [${CMAKE_PROJECT_NAME}]: Compiler is ${CMAKE_C_COMPILER_ID}, enabling exceptions with flag ${EXCEPTIONS_FLAG}")

# Enable throwing exceptions through C code
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXCEPTIONS_FLAG}")

# Ensure that C++ is using the same exception-handling flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXCEPTIONS_FLAG}")

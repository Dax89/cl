cmake_minimum_required(VERSION 3.15...3.23)

project(CmakeConfigPackageTests LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()
  find_package(cl CONFIG REQUIRED)

  if(NOT TARGET cl_options)
    message(FATAL_ERROR "Required config package not found!")
    return()
  endif()
endif()

include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)

add_executable(tests tests.cpp)

target_link_libraries(tests
    PRIVATE 
        Catch2::Catch2WithMain
        cl
)

catch_discover_tests(tests
  TEST_PREFIX "unittests."
  REPORTER XML
  OUTPUT_DIR .
  OUTPUT_PREFIX "unittests."
  OUTPUT_SUFFIX .xml
)

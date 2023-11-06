cmake_minimum_required(VERSION 3.15...3.23)

project(cl)

include(cmake/Dependencies.cmake)
setup_dependencies()

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} 
    INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED TRUE
)

if(NOT PROJECT_IS_TOP_LEVEL)
    return()
endif()

include(CTest)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

include(cmake/CPM.cmake)

function(setup_dependencies)
    if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING)
        CPMAddPackage("gh:catchorg/Catch2@3.5.0")
    endif()
endfunction()


include(cmake/CPM.cmake)

function(setup_dependencies)
    if(BUILD_TESTING)
        CPMAddPackage("gh:catchorg/Catch2@3.4.0")
    endif()
endfunction()


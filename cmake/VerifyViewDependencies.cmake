file(GLOB_RECURSE presentation_sources
    "${PROJECT_SOURCE_DIR}/Project1/src/presentation/*.hpp"
    "${PROJECT_SOURCE_DIR}/Project1/src/presentation/*.cpp")

foreach(source IN LISTS presentation_sources)
    if(source MATCHES "/viewmodel/" OR source MATCHES "/SfmlApplication\\.(hpp|cpp)$")
        continue()
    endif()

    file(READ "${source}" contents)
    if(contents MATCHES "#[ \t]*include[ \t]*\\\"(app|game|presentation/viewmodel)/")
        message(FATAL_ERROR "View boundary violation in ${source}: forbidden concrete-layer include")
    endif()
    if(contents MATCHES "(^|[^A-Za-z0-9_])(app|game)::")
        message(FATAL_ERROR "View boundary violation in ${source}: forbidden concrete-layer type")
    endif()
endforeach()

message(STATUS "SFML View dependency boundary is clean")

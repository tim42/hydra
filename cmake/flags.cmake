##
## CMAKE file for neam projects
##


if(MSVC)
    set(PROJECT_CXX_FLAGS /W4)
else()
    set(PROJECT_CXX_FLAGS -Wall -rdynamic)
endif()

# Workaround kdevelop refusing the C++23 standard if set in cmake...
#if(CMAKE_EXPORT_COMPILE_COMMANDS EQUAL ON)
    if(MSVC)
        # as of the writing of this file, it does not seems msvc has a flag for C++23
        set(PROJECT_CXX_FLAGS ${PROJECT_CXX_FLAGS} /std:c++latest)
    else()
        # kdevelop (and probably clang) requires 2b and not 23
        set(PROJECT_CXX_FLAGS ${PROJECT_CXX_FLAGS} -std=gnu++23 -Wno-tautological-compare -Wno-invalid-offsetof -Wno-unused-parameter -Wno-unused-function)
    endif()
#else()
    # this is the proper way, but kdevelop does not like it
    #set(CMAKE_CXX_STANDARD 23)
#endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
    if(MSVC)
    else()
        set(PROJECT_CXX_FLAGS ${PROJECT_CXX_FLAGS} -rdynamic -Og)
    endif()
endif()
if (${USE_TRACY})
    set(PROJECT_CXX_FLAGS ${PROJECT_CXX_FLAGS} -DTRACY_ENABLE=1)
endif()

project(cJSON C)

set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 7)
set(PROJECT_VERSION_PATCH 12)
set(CJSON_VERSION_SO 1)
set(CJSON_UTILS_VERSION_SO 1)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")


set(custom_compiler_flags)

include(CheckCCompilerFlag)
option(ENABLE_CUSTOM_COMPILER_FLAGS "Enables custom compiler flags" ON)
if (ENABLE_CUSTOM_COMPILER_FLAGS)
    if (("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU"))
        list(APPEND custom_compiler_flags
            -std=c89
            -pedantic
            -Wall
            -Wextra
            -Werror
            -Wstrict-prototypes
            -Wwrite-strings
            -Wshadow
            -Winit-self
            -Wcast-align
            -Wformat=2
            -Wmissing-prototypes
            -Wstrict-overflow=2
            -Wcast-qual
            -Wundef
            -Wswitch-default
            -Wconversion
            -Wc++-compat
            -fstack-protector-strong
            -Wcomma
            -Wdouble-promotion
            -Wparentheses
            -Wformat-overflow
            -Wunused-macros
            -Wmissing-variable-declarations
            -Wused-but-marked-unused
            -Wswitch-enum
            )
    elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
        # Disable warning c4001 - nonstandard extension 'single line comment' was used
        # Define _CRT_SECURE_NO_WARNINGS to disable deprecation warnings for "insecure" C library functions
        list(APPEND custom_compiler_flags
            /GS
            /Za
            /sdl
            /W4
            /wd4001
            /D_CRT_SECURE_NO_WARNINGS
            )
    endif()
endif()

option(ENABLE_PUBLIC_SYMBOLS "Export library symbols." On)
if (ENABLE_PUBLIC_SYMBOLS)
    list(APPEND custom_compiler_flags -fvisibility=hidden)
    add_definitions(-DCJSON_EXPORT_SYMBOLS -DCJSON_API_VISIBILITY)
endif()
option(ENABLE_HIDDEN_SYMBOLS "Hide library symbols." Off)
if (ENABLE_HIDDEN_SYMBOLS)
    add_definitions(-DCJSON_HIDE_SYMBOLS -UCJSON_API_VISIBILITY)
endif()

# apply custom compiler flags
foreach(compiler_flag ${custom_compiler_flags})
    #remove problematic characters
    string(REGEX REPLACE "[^a-zA-Z0-9]" "" current_variable ${compiler_flag})

    CHECK_C_COMPILER_FLAG(${compiler_flag} "FLAG_SUPPORTED_${current_variable}")
    if (FLAG_SUPPORTED_${current_variable})
        list(APPEND supported_compiler_flags)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${compiler_flag}")
    endif()
endforeach()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${supported_compiler_flags}")

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(ENABLE_TARGET_EXPORT "Enable exporting of CMake targets. Disable when it causes problems!" ON)

#cJSON
set(CJSON_LIB cjson)

file(GLOB HEADERS cJSON.h)
set(SOURCES cJSON.c)

set(CJSON_LIBRARY_TYPE STATIC)


add_library("${CJSON_LIB}" "${CJSON_LIBRARY_TYPE}" "${HEADERS}" "${SOURCES}")
target_link_libraries("${CJSON_LIB}" m)


set_target_properties("${CJSON_LIB}"
    PROPERTIES
    SOVERSION "${CJSON_VERSION_SO}"
    VERSION "${PROJECT_VERSION}")



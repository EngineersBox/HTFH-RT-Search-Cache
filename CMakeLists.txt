cmake_minimum_required(VERSION 3.21)
set(PROJECT_TARGET htfh_rt_search_cache)
project(${PROJECT_TARGET} C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE ON)

# ---- BUILDS ---- #

# Build Types
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}
        CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel tsan asan lsan msan ubsan"
        FORCE)

# Release
set(CMAKE_C_FLAGS_RELEASE
        "-O3 -pthread"
        CACHE STRING "Flags used by the C compile during release builds."
        FORCE)

# Debug
set(CMAKE_C_FLAGS_DEBUG
        "-O0 -Wall -pthread -g -fno-omit-frame-pointer"
        CACHE STRING "Flags used by the C compile during release builds."
        FORCE)

# ThreadSanitizer
set(CMAKE_C_FLAGS_TSAN
        "-fsanitize=thread -Wall  -g -pthread -O1 -g -fno-omit-frame-pointer"
        CACHE STRING "Flags used by the C compiler during ThreadSanitizer builds."
        FORCE)

# AddressSanitize
set(CMAKE_C_FLAGS_ASAN
        "-fsanitize=address -Wall  -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer -g -O1 -pthread"
        CACHE STRING "Flags used by the C compiler during AddressSanitizer builds."
        FORCE)

# LeakSanitizer
set(CMAKE_C_FLAGS_LSAN
        "-fsanitize=leak -Wall  -fno-omit-frame-pointer -g -O1 -pthread"
        CACHE STRING "Flags used by the C compiler during LeakSanitizer builds."
        FORCE)

# MemorySanitizer
set(CMAKE_C_FLAGS_MSAN
        "-fsanitize=memory -Wall  -fno-optimize-sibling-calls -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer -g -O2 -pthread"
        CACHE STRING "Flags used by the C compiler during MemorySanitizer builds."
        FORCE)

# UndefinedBehaviour
set(CMAKE_C_FLAGS_UBSAN
        "-O3 -fsanitize=undefined -Wall  -pthread -g -fno-omit-frame-pointer"
        CACHE STRING "Flags used by the C compiler during UndefinedBehaviourSanitizer builds."
        FORCE)


message("[${PROJECT_TARGET}] Configured build type: ${CMAKE_BUILD_TYPE}")

# ---- DEFINES ---- #

#add_definitions(
#        -DSTATIC_CFH
#        -DSTATIC_CFH_HEAP_SIZE=200000
#        -DSTATIC_CFH_CONSTRUCTOR_PRIORITY=0
#        -DSTATIC_CFH_DESTRUCTOR_PRIORITY=0
#)

# ---- SOURCES ---- #

# Include source content
file(GLOB_RECURSE sourceFiles CONFIGURE_DEPENDS "src/*.c")
file(GLOB_RECURSE headerFiles CONFIGURE_DEPENDS "src/*.h")

set(includeDirs "")
foreach(_headerFile ${headerFiles})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND includeDirs ${_dir})
endforeach()
list(REMOVE_DUPLICATES includeDirs)

# Mark executable
add_executable(${PROJECT_TARGET} ${sourceFiles})
target_include_directories(${PROJECT_TARGET} PRIVATE ${includeDirs})

# Global defines
target_compile_definitions(
        ${PROJECT_TARGET}
        PRIVATE
        ALLOCATOR_TYPE=0
#        HASH_FUNC=MEIYAN
        cache_backing_t=DLIRS*
        DQHT_ENABLE_STRICT
        DLIRS_ENABLE_STRICT
        ENABLE_LOGGING
        LOG_DATETIME_PREFIX
#        LOCK_DEBUG
)

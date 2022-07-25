cmake_minimum_required(VERSION 3.21)
project(htfh_rt_search_cache C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -pthread -g")
set(CMAKE_VERBOSE_MAKEFILE ON)

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
add_executable(htfh_rt_search_cache ${sourceFiles})
target_include_directories(htfh_rt_search_cache PRIVATE ${includeDirs})

# Global defines
target_compile_definitions(
        htfh_rt_search_cache
        PRIVATE
        HTFH_ALLOCATOR
        cache_backing_t=DLIRS*
        DQHT_ENABLE_STRICT
        DLIRS_ENABLE_STRICT
#        ENABLE_LOGGING
#        LOG_DATETIME_PREFIX
)
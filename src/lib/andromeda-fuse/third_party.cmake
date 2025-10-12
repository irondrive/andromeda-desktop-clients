
# include/link libfuse (LGPL 2.1 license)

option(LIBFUSE2 "Force using FUSE 2.x" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(FUSE REQUIRED)

if (${FUSE_MAJOR_VERSION} MATCHES 2)
    target_compile_definitions(libandromeda-fuse PRIVATE LIBFUSE2=1)
endif()

target_include_directories(libandromeda-fuse PRIVATE ${FUSE_INCLUDE_DIR})
target_link_libraries(libandromeda-fuse INTERFACE ${FUSE_LIBRARY})

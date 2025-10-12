cmake_minimum_required(VERSION 3.22)
project(libandromeda-fuse VERSION 1.0.0)

include(../../andromeda.cmake)

# build the andromeda-fuse library

set(SOURCE_FILES FuseAdapter.cpp FuseOperations.cpp FuseOptions.cpp)
andromeda_lib(libandromeda-fuse "${SOURCE_FILES}")

target_include_directories(libandromeda-fuse
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

install(TARGETS libandromeda-fuse
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# include/link libandromeda

if (NOT TARGET libandromeda)
    add_subdirectory(../andromeda lib/andromeda)
endif()

target_link_libraries(libandromeda-fuse PUBLIC libandromeda)

# add third party stuff
include(third_party.cmake)

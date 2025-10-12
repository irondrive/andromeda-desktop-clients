cmake_minimum_required(VERSION 3.22)
project(libandromeda VERSION 1.0.0)

include(../../andromeda.cmake)

# build the Andromeda library

set(SOURCE_FILES 
    BaseOptions.cpp
    Crypto.cpp
    Debug.cpp
    PlatformUtil.cpp
    SecureBuffer.cpp
    StringUtil.cpp
    )
andromeda_lib(libandromeda "${SOURCE_FILES}")

target_include_directories(libandromeda
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux") # needed for armv6
    target_link_libraries(libandromeda PUBLIC atomic)
endif()

install(TARGETS libandromeda
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_subdirectory(account)
add_subdirectory(backend)
add_subdirectory(database)
add_subdirectory(filesystem)

# add third party stuff
include(third_party.cmake)

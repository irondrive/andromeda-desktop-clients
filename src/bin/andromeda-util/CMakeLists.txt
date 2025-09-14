cmake_minimum_required(VERSION 3.22)
project(andromeda-util VERSION 1.0.0)

include(../../andromeda.cmake)

# build the andromeda-util executable

set(SOURCE_FILES main.cpp Options.cpp Actions.cpp)
andromeda_bin(andromeda-util "${SOURCE_FILES}")

install(TARGETS andromeda-util
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# include/link libandromeda

if (NOT TARGET libandromeda)
    add_subdirectory(../../lib/andromeda lib/andromeda)
endif()

target_link_libraries(andromeda-util PRIVATE libandromeda)

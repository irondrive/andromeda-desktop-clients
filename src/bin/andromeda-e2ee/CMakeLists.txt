cmake_minimum_required(VERSION 3.22)
project(andromeda-e2ee VERSION 1.0.0)

include(../../andromeda.cmake)

# build the andromeda-e2ee executable

set(SOURCE_FILES main.cpp Options.cpp Actions.cpp)
andromeda_bin(andromeda-e2ee "${SOURCE_FILES}")

install(TARGETS andromeda-e2ee
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# include/link libandromeda

if (NOT TARGET libandromeda)
    add_subdirectory(../../lib/andromeda lib/andromeda)
endif()

target_link_libraries(andromeda-e2ee PRIVATE libandromeda)

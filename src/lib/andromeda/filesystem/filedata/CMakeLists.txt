
option(FILEDATA_USE_MALLOC "Use malloc (not mmap) for file data pages" OFF)
if (FILEDATA_USE_MALLOC)
    target_compile_definitions(libandromeda PRIVATE FILEDATA_USE_MALLOC=1)
endif()

set(SOURCE_FILES 
    BandwidthMeasure.cpp
    CacheManager.cpp
    CacheOptions.cpp
    CachingAllocator.cpp
    MemoryAllocator.cpp
    Page.cpp
    PageBackend.cpp
    PageManager.cpp
    )

target_sources(libandromeda PRIVATE ${SOURCE_FILES})

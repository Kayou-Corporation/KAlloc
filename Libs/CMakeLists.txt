if (KALLOC_USE_TRACY)
    add_subdirectory(Tracy)
endif()

if(NOT TARGET KUtils)
    add_subdirectory(KUtils)
endif()
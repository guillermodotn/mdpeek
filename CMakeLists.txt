cmake_minimum_required(VERSION 3.16)
project(mdpeek VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

# --- GTK4 + WebKitGTK (system dependencies) ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(WEBKIT REQUIRED webkitgtk-6.0)

# --- cmark-gfm ---
# Try system-installed cmark-gfm first (via pkg-config),
# fall back to vendored submodule if not found.
pkg_check_modules(CMARK_GFM libcmark-gfm)

if(CMARK_GFM_FOUND)
    message(STATUS "Using system cmark-gfm ${CMARK_GFM_VERSION}")
else()
    message(STATUS "System cmark-gfm not found, using vendored submodule")
    set(CMARK_TESTS OFF CACHE BOOL "" FORCE)
    set(CMARK_SHARED OFF CACHE BOOL "" FORCE)
    set(CMARK_STATIC ON CACHE BOOL "" FORCE)
    add_subdirectory(vendor/cmark-gfm EXCLUDE_FROM_ALL)
endif()

# --- mdpeek executable ---
add_executable(mdpeek
    src/main.c
    src/viewer.c
)

target_include_directories(mdpeek PRIVATE
    ${GTK4_INCLUDE_DIRS}
    ${WEBKIT_INCLUDE_DIRS}
)

if(CMARK_GFM_FOUND)
    target_include_directories(mdpeek PRIVATE ${CMARK_GFM_INCLUDE_DIRS})
    target_link_libraries(mdpeek PRIVATE
        ${GTK4_LINK_LIBRARIES}
        ${WEBKIT_LINK_LIBRARIES}
        ${CMARK_GFM_LINK_LIBRARIES}
        cmark-gfm-extensions
    )
    target_compile_options(mdpeek PRIVATE
        ${GTK4_CFLAGS_OTHER}
        ${WEBKIT_CFLAGS_OTHER}
    )
else()
    target_include_directories(mdpeek PRIVATE
        ${CMAKE_BINARY_DIR}/vendor/cmark-gfm/src
        ${CMAKE_SOURCE_DIR}/vendor/cmark-gfm/src
        ${CMAKE_SOURCE_DIR}/vendor/cmark-gfm/extensions
    )
    target_link_libraries(mdpeek PRIVATE
        ${GTK4_LINK_LIBRARIES}
        ${WEBKIT_LINK_LIBRARIES}
        libcmark-gfm-extensions_static
        libcmark-gfm_static
    )
    target_compile_options(mdpeek PRIVATE
        ${GTK4_CFLAGS_OTHER}
        ${WEBKIT_CFLAGS_OTHER}
    )
endif()

install(TARGETS mdpeek DESTINATION ${CMAKE_INSTALL_BINDIR})

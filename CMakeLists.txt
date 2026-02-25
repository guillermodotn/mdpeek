cmake_minimum_required(VERSION 3.16)
project(mdpeek VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# --- Qt6 (system dependency) ---
find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets)

# --- cmark-gfm ---
# Try system-installed cmark-gfm first (via pkg-config),
# fall back to vendored submodule if not found.
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(CMARK_GFM libcmark-gfm)
endif()

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
    src/main.cpp
    src/MarkdownViewer.cpp
)

if(CMARK_GFM_FOUND)
    target_include_directories(mdpeek PRIVATE ${CMARK_GFM_INCLUDE_DIRS})
    target_link_libraries(mdpeek PRIVATE
        Qt6::Widgets
        Qt6::WebEngineWidgets
        ${CMARK_GFM_LINK_LIBRARIES}
        cmark-gfm-extensions
    )
else()
    target_include_directories(mdpeek PRIVATE
        ${CMAKE_BINARY_DIR}/vendor/cmark-gfm/src
        ${CMAKE_SOURCE_DIR}/vendor/cmark-gfm/src
        ${CMAKE_SOURCE_DIR}/vendor/cmark-gfm/extensions
    )
    target_link_libraries(mdpeek PRIVATE
        Qt6::Widgets
        Qt6::WebEngineWidgets
        libcmark-gfm-extensions_static
        libcmark-gfm_static
    )
endif()

install(TARGETS mdpeek DESTINATION ${CMAKE_INSTALL_BINDIR})

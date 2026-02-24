cmake_minimum_required(VERSION 3.16)
project(mdpeek VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# --- Qt6 (system dependency) ---
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# --- cmark-gfm (vendored submodule, static only) ---
set(CMARK_TESTS OFF CACHE BOOL "" FORCE)
set(CMARK_SHARED OFF CACHE BOOL "" FORCE)
set(CMARK_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(vendor/cmark-gfm)

# --- mdpeek executable ---
add_executable(mdpeek
    src/main.cpp
    src/MarkdownViewer.cpp
)

target_include_directories(mdpeek PRIVATE
    ${CMAKE_BINARY_DIR}/vendor/cmark-gfm/src
    ${CMAKE_SOURCE_DIR}/vendor/cmark-gfm/src
    ${CMAKE_SOURCE_DIR}/vendor/cmark-gfm/extensions
)

target_link_libraries(mdpeek PRIVATE
    Qt6::Widgets
    libcmark-gfm-extensions_static
    libcmark-gfm_static
)

cmake_minimum_required(VERSION 3.16)
project(mdpeek VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)

# --- Backend selection ---
set(MDPEEK_BACKEND "gtk" CACHE STRING "UI backend: gtk (default) or qt")
set_property(CACHE MDPEEK_BACKEND PROPERTY STRINGS gtk qt)
message(STATUS "Building with backend: ${MDPEEK_BACKEND}")

# --- cmark-gfm (shared by both backends) ---
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
    src/main.cpp
    src/viewer.cpp
)

# --- Backend-specific configuration ---
if(MDPEEK_BACKEND STREQUAL "qt")
    find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets)

    set_target_properties(mdpeek PROPERTIES AUTOMOC ON)
    target_compile_definitions(mdpeek PRIVATE MDPEEK_USE_QT)

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

else() # gtk (default)
    pkg_check_modules(GTK4    REQUIRED gtk4)
    pkg_check_modules(ADW     REQUIRED libadwaita-1)
    pkg_check_modules(WEBKIT  REQUIRED webkitgtk-6.0)

    target_include_directories(mdpeek PRIVATE
        ${GTK4_INCLUDE_DIRS}
        ${ADW_INCLUDE_DIRS}
        ${WEBKIT_INCLUDE_DIRS}
    )

    if(CMARK_GFM_FOUND)
        target_include_directories(mdpeek PRIVATE ${CMARK_GFM_INCLUDE_DIRS})
        target_link_libraries(mdpeek PRIVATE
            ${GTK4_LINK_LIBRARIES}
            ${ADW_LINK_LIBRARIES}
            ${WEBKIT_LINK_LIBRARIES}
            ${CMARK_GFM_LINK_LIBRARIES}
            cmark-gfm-extensions
        )
        target_compile_options(mdpeek PRIVATE
            ${GTK4_CFLAGS_OTHER}
            ${ADW_CFLAGS_OTHER}
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
            ${ADW_LINK_LIBRARIES}
            ${WEBKIT_LINK_LIBRARIES}
            libcmark-gfm-extensions_static
            libcmark-gfm_static
        )
        target_compile_options(mdpeek PRIVATE
            ${GTK4_CFLAGS_OTHER}
            ${ADW_CFLAGS_OTHER}
            ${WEBKIT_CFLAGS_OTHER}
        )
    endif()

endif()

install(TARGETS mdpeek DESTINATION ${CMAKE_INSTALL_BINDIR})

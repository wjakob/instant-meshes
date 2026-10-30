cmake_minimum_required(VERSION 3.15)
project(InstantMeshes LANGUAGES C CXX)

option(INSTANT_MESHES_DEV_MODE "Instant meshes developer mode" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

# Modern C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable folders for projects in Visual Studio
if (CMAKE_GENERATOR MATCHES "Visual Studio")
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

# Sanitize build environment for static build
if (MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS __TBB_NO_IMPLICIT_LINKAGE)
  # Parallel build on MSVC (all targets)
  add_compile_options(/MP)
  if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_compile_options(/arch:SSE2)
    # Disable Eigen vectorization for Windows 32 bit builds
    add_compile_definitions(EIGEN_DONT_ALIGN)
  endif()
  # Static runtime
  set(CompilerFlags
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
  foreach(CompilerFlag ${CompilerFlags})
    string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
  endforeach()
elseif(APPLE)
  # Try to auto-detect a suitable SDK
  # Commented out for now -- causes a too new SDK to be selected on AppVeyor
  # execute_process(COMMAND bash -c "xcodebuild -version -sdk | grep MacOSX | grep Path | head -n 1 | cut -f 2 -d ' '" OUTPUT_VARIABLE CMAKE_OSX_SYSROOT)
  # string(REGEX REPLACE "(\r?\n)+$" "" CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT}")
  # string(REGEX REPLACE "^.*X([0-9.]*).sdk$" "\\1" CMAKE_OSX_DEPLOYMENT_TARGET "${CMAKE_OSX_SYSROOT}")
endif()

# Find system dependencies
find_package(Eigen3 3.4 REQUIRED)
find_package(glfw3 3.3 REQUIRED)

# Build NanoGUI (without its bundled dependencies)
set(NANOGUI_BUILD_EXAMPLE OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_SHARED  OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_PYTHON  OFF CACHE BOOL " " FORCE)
# Windows needs the bundled GLAD OpenGL loader; Linux/macOS use system GL headers
if (NOT WIN32)
  set(NANOGUI_USE_GLAD OFF CACHE BOOL " " FORCE)
endif()
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui ext_build/nanogui)
set_property(TARGET nanogui glfw bin2c PROPERTY FOLDER "dependencies")
add_definitions(${NANOGUI_EXTRA_DEFS})

# Build TBB (bundled)
set(TBB_BUILD_STATIC ON CACHE BOOL " " FORCE)
set(TBB_BUILD_SHARED OFF CACHE BOOL " " FORCE)
set(TBB_BUILD_TBBMALLOC OFF CACHE BOOL " " FORCE)
set(TBB_BUILD_TBBMALLOC_PROXY OFF CACHE BOOL " " FORCE)
set(TBB_BUILD_TESTS OFF CACHE BOOL " " FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ext/tbb ext_build/tbb)
set_property(TARGET tbb_static tbb_def_files PROPERTY FOLDER "dependencies")
# Suppress -Wchanges-meaning in bundled TBB (fixed in newer TBB versions)
if (NOT MSVC)
  target_compile_options(tbb_static PRIVATE -Wno-changes-meaning)
endif()

# Compiler warnings for InstantMeshes target
if(MSVC)
  add_compile_options(/W4)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-changes-meaning)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-gnu-anonymous-struct -Wno-c99-extensions -Wno-nested-anon-types)
  endif()
endif()

# Header file directories
include_directories(
  # NanoGUI
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/include
  # System dependencies
  ${NANOGUI_EXTRA_INCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/tbb/include
  ${GLFW_INCLUDE_DIRS}
  # Local dependencies
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/dset
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/pss
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/pcg32
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/rply
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/half
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/ext/nanovg/src
  # Resource file
  ${CMAKE_CURRENT_BINARY_DIR}
)

if (INSTANT_MESHES_DEV_MODE)
  add_compile_definitions(DEV_MODE)
endif()

# Resource file (icons etc.)
set(EXTRA_SOURCE "")
if (APPLE)
  set(EXTRA_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resources/im.icns"
                   "${CMAKE_CURRENT_SOURCE_DIR}/src/native_file_dialog.cpp")
elseif(LINUX)
  set(EXTRA_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/native_file_dialog.cpp")
elseif(WIN32)
  set(EXTRA_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resources/im.rc")
endif()

# Run bin2c on resource files
set(PNG_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/comb.png
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/delete_stroke.png
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/loadmesh.png
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/scare.png
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/sing_dir.png
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/sing_pos.png
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/instantmeshes.png)
file(GLOB FRAG_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.frag")
file(GLOB VERT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.vert")
file(GLOB GEO_FILES  "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.geo")
set(IM_RESOURCES ${PNG_FILES} ${FRAG_FILES} ${VERT_FILES} ${GEO_FILES})

add_custom_command(
  OUTPUT resources.cpp
  COMMAND bin2c ARGS resources.cpp resources.h ${IM_RESOURCES}
  DEPENDS bin2c ${IM_RESOURCES}
  COMMENT "Running bin2c"
  VERBATIM)

add_executable(InstantMeshes MACOSX_BUNDLE
  resources.cpp
  src/glutil.h src/glutil.cpp
  src/widgets.h src/widgets.cpp
  src/aabb.h
  src/main.cpp src/common.h
  src/meshio.h src/meshio.cpp
  src/normal.h src/normal.cpp
  src/adjacency.h src/adjacency.cpp
  src/meshstats.h src/meshstats.cpp
  src/hierarchy.h src/hierarchy.cpp
  src/extract.h src/extract.cpp
  src/viewer.h src/viewer.cpp
  src/field.h src/field.cpp
  src/bvh.h src/bvh.cpp
  src/subdivide.h src/subdivide.cpp
  src/reorder.h src/reorder.cpp
  src/serializer.h src/serializer.cpp
  src/batch.h src/batch.cpp
  src/smoothcurve.h src/smoothcurve.cpp
  src/cleanup.h src/cleanup.cpp
  src/dedge.h src/dedge.cpp
  ext/rply/rply.c
  ${EXTRA_SOURCE})

target_link_libraries(InstantMeshes PRIVATE Eigen3::Eigen tbb_static nanogui ${GLFW_LIBRARIES} ${NANOGUI_EXTRA_LIBS})
target_include_directories(InstantMeshes PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/include)

set_target_properties(InstantMeshes PROPERTIES OUTPUT_NAME "Instant Meshes")

if (WIN32)
  if (MSVC)
    set_source_files_properties(ext/rply/rply.c PROPERTIES COMPILE_FLAGS "/wd4127")
  endif()
elseif (APPLE)
  set_target_properties(InstantMeshes PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "Instant Meshes"
    MACOSX_BUNDLE_BUNDLE_GUI_IDENTIFIER "com.im.InstantMeshes"
    MACOSX_BUNDLE_ICON_FILE im.icns
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/MacOSXBundleInfo.plist.in)
  set_source_files_properties(resources/im.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
else()
  # Insulate from a few types of ABI changes by statically linking against libgcc
  target_link_options(InstantMeshes PRIVATE -static-libgcc)
endif()

if (UNIX AND NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
  add_custom_command(TARGET InstantMeshes POST_BUILD COMMAND strip $<TARGET_FILE:InstantMeshes>)
endif()
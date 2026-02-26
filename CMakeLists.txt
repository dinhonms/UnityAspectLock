cmake_minimum_required(VERSION 3.8)
project(UnityAspectLock LANGUAGES CXX)

# This project builds a Win32 DLL that locks a Unity window to a fixed aspect ratio.
# It must be built on Windows or using a cross-compiler that targets Windows (e.g. MinGW-w64).

add_library(UnityAspectLock SHARED UnityAspectLock.cpp)

# Ensure the same macros used in the original source.
target_compile_definitions(UnityAspectLock PRIVATE WIN32_LEAN_AND_MEAN)

# Link against common controls for SetWindowSubclass, etc.
target_link_libraries(UnityAspectLock PRIVATE Comctl32)

# Set the output name to match what Unity expects (optional)
set_target_properties(UnityAspectLock PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON    # export all extern "C" symbols automatically
    OUTPUT_NAME "UnityAspectLock"   # optional explicit name
)

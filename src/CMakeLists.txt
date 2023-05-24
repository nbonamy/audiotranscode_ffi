# The Flutter tooling requires that developers have CMake 3.10 or later
# installed. You should not increase this version, as doing so will cause
# the plugin to fail to compile for some customers of the plugin.
cmake_minimum_required(VERSION 3.10)

project(audiotranscode_ffi_library VERSION 0.0.1 LANGUAGES CXX)

add_library(
  audiotranscode_ffi SHARED "audiotranscode_ffi.cxx" "AudioTranscoder.cxx" "FlacMetadata.cxx"
)

set_target_properties(audiotranscode_ffi PROPERTIES
  PUBLIC_HEADER audiotranscode_ffi.h
  OUTPUT_NAME "audiotranscode_ffi"
  LINKER_LANGUAGE CXX
)

target_include_directories(audiotranscode_ffi PUBLIC
  "$ENV{HOMEBREW_PREFIX}/include"
)

target_compile_definitions(audiotranscode_ffi PUBLIC DART_SHARED_LIB)

target_link_libraries(audiotranscode_ffi PRIVATE
  "$ENV{HOMEBREW_PREFIX}/lib/libavcodec.dylib"
  "$ENV{HOMEBREW_PREFIX}/lib/libavformat.dylib"
  "$ENV{HOMEBREW_PREFIX}/lib/libavutil.dylib"
  "$ENV{HOMEBREW_PREFIX}/lib/libswresample.dylib"
  "$ENV{HOMEBREW_PREFIX}/lib/libFLAC.dylib"
)

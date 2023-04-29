#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

#if _WIN32
#define FFI_PLUGIN_EXPORT __declspec(dllexport)
#else
#define FFI_PLUGIN_EXPORT
#endif

#ifndef __cplusplus
#define bool int
#endif

#ifdef __cplusplus
extern "C" {
#endif

  FFI_PLUGIN_EXPORT bool transcode_to_mp3(const char* src, const char* dst, int bitrate);
  FFI_PLUGIN_EXPORT bool transcode_to_flac(const char* src, const char* dst, int samplerate, int bitspersample);

#ifdef __cplusplus
}
#endif

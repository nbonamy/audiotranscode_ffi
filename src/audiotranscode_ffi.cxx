#include "audiotranscode_ffi.h"
#include "AudioTranscoder.h"

FFI_PLUGIN_EXPORT bool transcode_to_mp3(const char* src, const char* dst, int bitrate)
{
  CAudioTranscoder transcoder;
  int rc = transcoder.Transcode(src, dst, CAudioTranscoder::Mp3, 0, 0, bitrate);
  return (rc == 0);
}

FFI_PLUGIN_EXPORT bool transcode_to_flac(const char* src, const char* dst, int samplerate, int bitspersample)
{
  CAudioTranscoder transcoder;
  int rc = transcoder.Transcode(src, dst, CAudioTranscoder::Flac, bitspersample, samplerate, 0);
  return (rc == 0);
}

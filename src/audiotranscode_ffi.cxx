#include "audiotranscode_ffi.h"
#include "AudioTranscoder.h"

FFI_PLUGIN_EXPORT bool transcode_to_mp3(const char* src, const char* dst, int bitrate)
{
  CAudioTranscoder transcoder;
  int rc = transcoder.Transcode(src, dst, CAudioTranscoder::Mp3, bitrate, 0, 0);
  return (rc == 0);
}

FFI_PLUGIN_EXPORT bool transcode_to_aac(const char* src, const char* dst, int bitrate)
{
  CAudioTranscoder transcoder;
  int rc = transcoder.Transcode(src, dst, CAudioTranscoder::Aac, bitrate, 0, 0);
  return (rc == 0);
}

FFI_PLUGIN_EXPORT bool transcode_to_flac(const char* src, const char* dst, int bits_per_sample, int sample_rate)
{
  CAudioTranscoder transcoder;
  int rc = transcoder.Transcode(src, dst, CAudioTranscoder::Flac, 0, bits_per_sample, sample_rate);
  return (rc == 0);
}

FFI_PLUGIN_EXPORT bool transcode_to_alac(const char* src, const char* dst, int bits_per_sample, int sample_rate)
{
  CAudioTranscoder transcoder;
  int rc = transcoder.Transcode(src, dst, CAudioTranscoder::Alac, 0, bits_per_sample, sample_rate);
  return (rc == 0);
}

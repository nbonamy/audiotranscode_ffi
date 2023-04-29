#pragma once
#include <string>

extern "C"
{
  struct AVFormatContext;
  struct AVCodecContext;
  struct AVPacket;
  struct AVFrame;
  struct AVAudioFifo;
  struct SwrContext;
}

class CAudioTranscoder
{

public:
  enum TargetFormat
  {
    Mp3,
    Flac,
  };

public:
  CAudioTranscoder();
  virtual ~CAudioTranscoder();

  bool Transcode(
    const std::string &inputFile, const std::string &outputFile,
    TargetFormat targetFormat, int bits_per_sample, int sample_rate, int bitrate
  );

private:
  void init_transcoding_params(TargetFormat targetFormat, int bits_per_sample, int sample_rate, int bitrate);

private:
  int transcode(const char *input_filename, const char *output_filename);
  int open_input_file(const char *filename, AVFormatContext **input_format_context, AVCodecContext **input_codec_context);
  int open_output_file(const char *filename, AVCodecContext *input_codec_context, AVFormatContext **output_format_context, AVCodecContext **output_codec_context);

  void init_packet(AVPacket **packet);
  int init_input_frame(AVFrame **frame);
  int init_resampler(AVCodecContext *input_codec_context, AVCodecContext *output_codec_context, SwrContext **resample_context);
  int init_fifo(AVAudioFifo **fifo, AVCodecContext *output_codec_context);

  int write_output_file_header(AVFormatContext *output_format_context);
  int write_output_file_trailer(AVFormatContext *output_format_context);

  int read_decode_convert_and_store(AVAudioFifo *fifo,
                                    AVFormatContext *input_format_context,
                                    AVCodecContext *input_codec_context,
                                    AVCodecContext *output_codec_context,
                                    SwrContext *resampler_context,
                                    int *finished);

  int decode_audio_frame(AVFrame *frame,
                         AVFormatContext *input_format_context,
                         AVCodecContext *input_codec_context,
                         int *data_present, int *finished);

  int init_converted_samples(uint8_t ***converted_input_samples,
                             AVCodecContext *output_codec_context,
                             int frame_size);

  int convert_samples(const uint8_t **input_data,
                      uint8_t **converted_data, const int frame_size,
                      SwrContext *resample_context);

  int add_samples_to_fifo(AVAudioFifo *fifo,
                          uint8_t **converted_input_samples,
                          const int frame_size);

  int load_encode_and_write(AVAudioFifo *fifo,
                            AVFormatContext *output_format_context,
                            AVCodecContext *output_codec_context);

  int init_output_frame(AVFrame **frame,
                        AVCodecContext *output_codec_context,
                        int frame_size);

  int encode_audio_frame(AVFrame *frame,
                         AVFormatContext *output_format_context,
                         AVCodecContext *output_codec_context,
                         int *data_present);

private:
  std::string av_err2string(int errnum);

private:
  int64_t pts; // global timestamp for the audio frames
  int codec_id;
  int sample_format;
  int sample_rate;
  int bitrate;
  int bits_per_sample;
  int audio_stream_index;
};
